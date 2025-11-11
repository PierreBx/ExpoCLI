#!/bin/bash
# expocli.sh - Transparent wrapper for expocli Docker container
# This script automatically builds and runs expocli inside a Docker container
# while making it feel like a native command.

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTAINER_BUILD_DIR="/app/build"
CONTAINER_BINARY="${CONTAINER_BUILD_DIR}/expocli"

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed. Please install Docker first." >&2
    exit 1
fi

# Check if Docker Compose is available
if ! docker compose version &> /dev/null 2>&1; then
    echo "Error: Docker Compose V2 is not available. Please update Docker." >&2
    exit 1
fi

# Check if we need to build the image (try to run a simple test)
check_and_build_image() {
    cd "${SCRIPT_DIR}"
    if ! docker compose run --rm --no-TTY expocli true 2>/dev/null; then
        echo "[expocli] Building Docker image..." >&2
        docker compose build >&2
        echo "[expocli] Docker image built successfully." >&2
    fi
}

# Check if binary exists and build if needed
check_and_build_binary() {
    cd "${SCRIPT_DIR}"
    if ! docker compose run --rm --no-TTY expocli test -f "${CONTAINER_BINARY}" 2>/dev/null; then
        echo "[expocli] Compiling expocli binary..." >&2
        docker compose run --rm --no-TTY expocli bash -c \
            "cd ${CONTAINER_BUILD_DIR} && cmake .. >/dev/null 2>&1 && make >/dev/null 2>&1" >&2

        if docker compose run --rm --no-TTY expocli test -f "${CONTAINER_BINARY}" 2>/dev/null; then
            echo "[expocli] Compilation successful." >&2
        else
            echo "Error: Failed to compile expocli." >&2
            exit 1
        fi
    fi
}

# Main execution
main() {
    # Ensure everything is set up
    check_and_build_image
    check_and_build_binary

    # Determine if we need TTY (interactive mode)
    TTY_FLAG=""
    if [ -t 0 ] && [ -t 1 ]; then
        TTY_FLAG="-it"
    fi

    # Get the current working directory to mount
    HOST_CWD="$(pwd)"

    # Run expocli inside container with:
    # - Current directory mounted as /workspace
    # - User's arguments passed through
    # - TTY allocated if interactive
    # - Exit code preserved
    cd "${SCRIPT_DIR}"

    docker compose run --rm ${TTY_FLAG} \
        -v "${HOST_CWD}:/workspace:rw" \
        -w /workspace \
        expocli \
        ${CONTAINER_BINARY} "$@"

    exit $?
}

# Run main function with all arguments
main "$@"
