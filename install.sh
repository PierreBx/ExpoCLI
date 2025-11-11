#!/bin/bash
# install.sh - Install expocli wrapper for easy system-wide access
#
# Usage:
#   ./install.sh              # Install with existing Docker image
#   ./install.sh --rebuild-docker  # Install and rebuild Docker image

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WRAPPER_SCRIPT="${SCRIPT_DIR}/expocli.sh"

# Parse command line arguments
REBUILD_DOCKER=false
for arg in "$@"; do
    case $arg in
        --rebuild-docker)
            REBUILD_DOCKER=true
            shift
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Usage: ./install.sh [--rebuild-docker]"
            exit 1
            ;;
    esac
done

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║        expocli Installer               ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

# Detect user's shell
detect_shell() {
    if [ -n "$ZSH_VERSION" ]; then
        echo "zsh"
    elif [ -n "$BASH_VERSION" ]; then
        echo "bash"
    else
        echo "bash"  # Default to bash
    fi
}

SHELL_TYPE=$(detect_shell)

# Determine shell RC file
case "$SHELL_TYPE" in
    zsh)
        SHELL_RC="${HOME}/.zshrc"
        ;;
    bash)
        if [ -f "${HOME}/.bashrc" ]; then
            SHELL_RC="${HOME}/.bashrc"
        else
            SHELL_RC="${HOME}/.bash_profile"
        fi
        ;;
    *)
        SHELL_RC="${HOME}/.bashrc"
        ;;
esac

echo "Detected shell: $SHELL_TYPE"
echo "Shell RC file: $SHELL_RC"
echo ""

# Create the alias
ALIAS_CMD="alias expocli='${WRAPPER_SCRIPT}'"

# Check if alias already exists
echo "Checking expocli alias status..."
if grep -q "alias expocli=" "$SHELL_RC" 2>/dev/null; then
    # Check if the existing alias points to the correct location
    EXISTING_ALIAS=$(grep "alias expocli=" "$SHELL_RC" | tail -n 1)
    if echo "$EXISTING_ALIAS" | grep -q "'${WRAPPER_SCRIPT}'"; then
        echo -e "${GREEN}✓${NC} expocli alias already configured correctly in $SHELL_RC"
        ALIAS_STATUS="already_exists"
    else
        # Alias exists but points to a different location - update it
        echo -e "${YELLOW}⚠${NC}  expocli alias exists but points to different location"
        echo "    Updating to: ${WRAPPER_SCRIPT}"
        sed -i.bak '/alias expocli=/d' "$SHELL_RC"
        echo "" >> "$SHELL_RC"
        echo "# expocli - XML Query CLI (transparent Docker wrapper)" >> "$SHELL_RC"
        echo "$ALIAS_CMD" >> "$SHELL_RC"
        echo -e "${GREEN}✓${NC} Updated expocli alias in $SHELL_RC"
        ALIAS_STATUS="updated"
    fi
else
    # Alias doesn't exist - add it
    echo "" >> "$SHELL_RC"
    echo "# expocli - XML Query CLI (transparent Docker wrapper)" >> "$SHELL_RC"
    echo "$ALIAS_CMD" >> "$SHELL_RC"
    echo -e "${GREEN}✓${NC} Added expocli alias to $SHELL_RC"
    ALIAS_STATUS="added"
fi
echo ""

# Make wrapper script executable (if not already)
chmod +x "${WRAPPER_SCRIPT}"

# Build and compilation process
echo ""
echo -e "${BLUE}═══════════════════════════════════════${NC}"
echo -e "${BLUE}  Docker Build & Compilation Process  ${NC}"
echo -e "${BLUE}═══════════════════════════════════════${NC}"
echo ""

cd "${SCRIPT_DIR}"

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo -e "${YELLOW}⚠${NC}  Docker not found!"
    echo "    Docker is required for expocli to work."
    echo "    Please install Docker from: https://docs.docker.com/get-docker/"
    echo ""
    echo "    Installation incomplete - Docker required."
    exit 1
fi

# Check if Docker daemon is running
if ! docker ps &> /dev/null; then
    echo -e "${YELLOW}⚠${NC}  Docker daemon is not running!"
    echo "    Please start Docker and try again."
    echo ""
    echo "    Installation incomplete - Docker not running."
    exit 1
fi

# Check if Docker Compose is available
if ! docker compose version &> /dev/null 2>&1; then
    echo -e "${YELLOW}⚠${NC}  Docker Compose V2 is not available!"
    echo "    Please update Docker to get Compose V2."
    echo ""
    echo "    Installation incomplete - Docker Compose required."
    exit 1
fi

echo -e "${GREEN}✓${NC} Docker is available and running"
echo ""

# Now proceed with build/compile
    if [ "$REBUILD_DOCKER" = true ]; then
        # Full rebuild mode - stop containers and rebuild image
        echo -e "${YELLOW}INFO:${NC} Running with --rebuild-docker flag"
        echo ""

        # Step 1: Stop any running containers
        echo -e "${BLUE}[1/5]${NC} Stopping any running containers..."
        CONTAINER_NAME="expocli_container"
        if docker ps -q -f name="${CONTAINER_NAME}" | grep -q .; then
            echo "      Stopping expocli_container..."
            docker compose down 2>/dev/null || true
            echo -e "${GREEN}✓${NC} Stopped running container"
        else
            echo -e "${GREEN}✓${NC} No running containers to stop"
        fi

        # Step 2: Rebuild Docker image
        echo ""
        echo -e "${BLUE}[2/5]${NC} Rebuilding Docker image with latest dependencies..."
        echo "      (This includes readline library and other dependencies)"
        docker compose build --no-cache
        echo -e "${GREEN}✓${NC} Docker image rebuilt successfully"

        # Step 3: Start the persistent container
        echo ""
        echo -e "${BLUE}[3/5]${NC} Starting persistent container..."
        docker compose up -d
        sleep 2  # Give container time to be ready
        if docker ps -q -f name="${CONTAINER_NAME}" -f status=running | grep -q .; then
            echo -e "${GREEN}✓${NC} Container is running"
        else
            echo -e "${YELLOW}⚠${NC}  Failed to start container"
            exit 1
        fi

        # Step 4: Clean old build directory
        echo ""
        echo -e "${BLUE}[4/5]${NC} Cleaning old build artifacts..."
        if docker compose exec -T expocli bash -c "rm -rf /app/build/* /app/build/.* 2>/dev/null || true"; then
            echo -e "${GREEN}✓${NC} Build directory cleaned"
        else
            echo -e "${YELLOW}⚠${NC}  Warning: Could not clean build directory, but continuing..."
        fi

        # Step 5: Compile the binary with latest code
        echo ""
        echo -e "${BLUE}[5/5]${NC} Compiling expocli with latest source code..."
        echo "      (This may take 30-60 seconds)"

        if docker compose exec -T expocli bash -c \
            "mkdir -p /app/build && cd /app/build && cmake .. >/dev/null 2>&1 && make"; then
            echo ""
            # Verify the binary was created
            if docker compose exec -T expocli test -f /app/build/expocli 2>/dev/null; then
                echo -e "${GREEN}✓${NC} Compilation successful"
            else
                echo -e "${YELLOW}⚠${NC}  Binary not found after compilation"
                exit 1
            fi
        else
            echo ""
            echo -e "${YELLOW}⚠${NC}  Compilation failed!"
            exit 1
        fi

        echo ""
        echo -e "${GREEN}✓${NC} Docker rebuild and compilation complete!"
    else
        # Quick rebuild mode - just recompile binary with existing image
        echo -e "${YELLOW}INFO:${NC} Using existing Docker image (use --rebuild-docker to rebuild)"
        echo ""

        # Ensure Docker image exists
        echo "Testing Docker setup..."
        if ! docker images -q expocli_image | grep -q .; then
            echo -e "${BLUE}[Building]${NC} Docker image not found, building it now..."
            echo "           (This is a one-time setup, takes ~1-2 minutes)"
            if docker compose build; then
                echo -e "${GREEN}✓${NC} Docker image built successfully"
                echo ""
            else
                echo -e "${YELLOW}⚠${NC}  Docker build failed!"
                exit 1
            fi
        else
            echo -e "${GREEN}✓${NC} Docker image exists"
            echo ""
        fi

        # Start the persistent container if not already running
        CONTAINER_NAME="expocli_container"
        if docker ps -q -f name="${CONTAINER_NAME}" -f status=running | grep -q .; then
            echo -e "${GREEN}✓${NC} Container is already running"
            echo ""
        else
            echo -e "${BLUE}[Starting]${NC} Starting persistent container..."
            docker compose up -d
            sleep 2  # Give container time to be ready
            if docker ps -q -f name="${CONTAINER_NAME}" -f status=running | grep -q .; then
                echo -e "${GREEN}✓${NC} Container is running"
                echo ""
            else
                echo -e "${YELLOW}⚠${NC}  Failed to start container"
                exit 1
            fi
        fi

        # Step 1: Clean old build directory
        echo -e "${BLUE}[1/2]${NC} Cleaning old build artifacts..."
        if docker compose exec -T expocli bash -c "rm -rf /app/build/* /app/build/.* 2>/dev/null || true"; then
            echo -e "${GREEN}✓${NC} Build directory cleaned"
        else
            echo -e "${YELLOW}⚠${NC}  Warning: Could not clean build directory, but continuing..."
        fi

        # Step 2: Compile the binary with latest code
        echo ""
        echo -e "${BLUE}[2/2]${NC} Compiling expocli with latest source code..."
        echo "      (This may take 30-60 seconds)"

        if docker compose exec -T expocli bash -c \
            "mkdir -p /app/build && cd /app/build && cmake .. >/dev/null 2>&1 && make"; then
            echo ""
            # Verify the binary was created
            if docker compose exec -T expocli test -f /app/build/expocli 2>/dev/null; then
                echo -e "${GREEN}✓${NC} Compilation successful"
            else
                echo -e "${YELLOW}⚠${NC}  Binary not found after compilation"
                exit 1
            fi
        else
            echo ""
            echo -e "${YELLOW}⚠${NC}  Compilation failed!"
            exit 1
        fi

        echo ""
        echo -e "${GREEN}✓${NC} Binary compilation complete!"
    fi

# Test the setup
echo ""
echo "Testing setup..."
cd "${SCRIPT_DIR}"

if bash "${WRAPPER_SCRIPT}" --help >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} expocli wrapper is working correctly"
else
    echo -e "${YELLOW}⚠${NC}  Initial setup may take a moment (building Docker image and binary)"
fi

echo ""
echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     Installation Complete! 🎉         ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
echo ""
echo "To start using expocli, run:"
echo ""
echo -e "  ${BLUE}source $SHELL_RC${NC}"
echo -e "  ${BLUE}expocli${NC}"
echo ""
echo "Or open a new terminal window."
echo ""
echo "Usage examples:"
echo -e "  ${BLUE}expocli${NC}                                    # Start interactive mode"
echo -e "  ${BLUE}expocli 'SELECT name FROM ./data'${NC}         # Single query"
echo -e "  ${BLUE}expocli --help${NC}                             # Show help"
echo ""
echo -e "${BLUE}How it works:${NC}"
echo "  expocli runs inside a persistent Docker container transparently."
echo "  The container:"
echo "    - Is now running in the background (very lightweight)"
echo "    - Will restart automatically if stopped"
echo "    - Executes queries instantly (no container startup overhead)"
echo "  You won't even notice - it feels like a native command!"
echo ""
if [ "$REBUILD_DOCKER" = true ]; then
    echo "Note: Docker image has been rebuilt with latest dependencies."
    echo "      Binary has been compiled with the latest source code."
    echo "      Persistent container is running and ready!"
else
    echo "Note: Binary has been compiled with the latest source code."
    echo "      Persistent container is running and ready!"
    echo "      To rebuild Docker image: ./install.sh --rebuild-docker"
fi
echo ""
echo "Tip:  Check container status: cd $(pwd) && docker compose ps"
echo "      Stop container: docker compose down (auto-restarts on next use)"
echo ""
