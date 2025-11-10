#!/bin/bash
# install.sh - Install expocli wrapper for easy system-wide access

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WRAPPER_SCRIPT="${SCRIPT_DIR}/expocli.sh"

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
if grep -q "alias expocli=" "$SHELL_RC" 2>/dev/null; then
    echo -e "${YELLOW}⚠${NC}  expocli alias already exists in $SHELL_RC"
    read -p "Do you want to update it? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # Remove old alias
        sed -i.bak '/alias expocli=/d' "$SHELL_RC"
        echo -e "${GREEN}✓${NC} Removed old alias"
    else
        echo "Installation cancelled."
        exit 0
    fi
fi

# Add alias to shell RC
echo "" >> "$SHELL_RC"
echo "# expocli - XML Query CLI (transparent Docker wrapper)" >> "$SHELL_RC"
echo "$ALIAS_CMD" >> "$SHELL_RC"

echo -e "${GREEN}✓${NC} Added expocli alias to $SHELL_RC"
echo ""

# Make wrapper script executable (if not already)
chmod +x "${WRAPPER_SCRIPT}"

# Test the setup
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
echo "The first run will:"
echo "  • Build the Docker image (if not already built)"
echo "  • Compile expocli (if not already compiled)"
echo "  • This may take 1-2 minutes"
echo ""
echo "Subsequent runs will be instant!"
echo ""
