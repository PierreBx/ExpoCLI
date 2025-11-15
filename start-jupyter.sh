#!/bin/bash
#
# ExpoCLI Jupyter Lab Startup Script
#
# This script starts Jupyter Lab in the Docker container with proper configuration
# for the ExpoCLI kernel integration.
#

set -e

echo "========================================="
echo "ExpoCLI Jupyter Lab Environment"
echo "========================================="
echo ""

# Verify ExpoCLI binary is available
if [ ! -f "/app/build/expocli" ]; then
    echo "ERROR: ExpoCLI binary not found at /app/build/expocli"
    echo "The C++ project may not have been built correctly."
    exit 1
fi

echo "✓ ExpoCLI binary found at /app/build/expocli"

# Verify the kernel is installed
if ! jupyter kernelspec list | grep -q expocli; then
    echo "ERROR: ExpoCLI kernel not installed"
    echo "Installing the kernel now..."
    python3 -m expocli_kernel.install
fi

echo "✓ ExpoCLI Jupyter kernel is installed"
echo ""

# Display kernel information
echo "Available Jupyter kernels:"
jupyter kernelspec list
echo ""

# Generate Jupyter configuration if it doesn't exist
JUPYTER_CONFIG_DIR="${HOME}/.jupyter"
mkdir -p "${JUPYTER_CONFIG_DIR}"

if [ ! -f "${JUPYTER_CONFIG_DIR}/jupyter_lab_config.py" ]; then
    echo "Generating Jupyter Lab configuration..."
    jupyter lab --generate-config
fi

# Start Jupyter Lab
echo "========================================="
echo "Starting Jupyter Lab..."
echo "========================================="
echo ""
echo "Access Jupyter Lab at: http://localhost:8888"
echo ""
echo "The demo notebook is available at:"
echo "  examples/ExpoCLI_Demo.ipynb"
echo ""
echo "Press Ctrl+C to stop Jupyter Lab"
echo ""

# Start Jupyter Lab with appropriate settings
exec jupyter lab \
    --ip=0.0.0.0 \
    --port=8888 \
    --no-browser \
    --allow-root \
    --NotebookApp.token='' \
    --NotebookApp.password='' \
    --NotebookApp.allow_origin='*' \
    --NotebookApp.base_url='/'
