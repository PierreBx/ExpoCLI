#!/usr/bin/env python3
"""
Installation script for ExpoCLI Jupyter kernel
"""

import json
import os
import sys
import shutil
import subprocess
from pathlib import Path


def get_jupyter_data_dir():
    """Get the Jupyter data directory"""
    try:
        result = subprocess.run(
            ['jupyter', '--data-dir'],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        # Fallback to default location
        if sys.platform == 'win32':
            return os.path.join(os.environ.get('APPDATA', ''), 'jupyter')
        else:
            return os.path.expanduser('~/.local/share/jupyter')


def install_kernelspec():
    """Install the ExpoCLI kernel specification"""
    # Get paths
    kernel_dir = Path(__file__).parent
    kernelspec_dir = kernel_dir / 'kernelspec'
    jupyter_data_dir = Path(get_jupyter_data_dir())
    install_dir = jupyter_data_dir / 'kernels' / 'expocli'

    # Create installation directory
    install_dir.mkdir(parents=True, exist_ok=True)

    # Copy kernel.json
    kernel_json = kernelspec_dir / 'kernel.json'
    if kernel_json.exists():
        shutil.copy2(kernel_json, install_dir / 'kernel.json')
        print(f"✓ Copied kernel.json to {install_dir}")
    else:
        print(f"✗ Error: kernel.json not found at {kernel_json}")
        return False

    # Create a logo if we had one (optional for POC)
    # For now, just note that we could add one later

    print(f"\n✓ ExpoCLI kernel installed successfully!")
    print(f"  Location: {install_dir}")
    print(f"\nTo verify installation:")
    print(f"  jupyter kernelspec list")
    print(f"\nTo start using:")
    print(f"  jupyter notebook")
    print(f"  # or")
    print(f"  jupyter lab")
    print(f"\nThen select 'ExpoCLI' from the kernel list when creating a new notebook.")

    return True


def uninstall_kernelspec():
    """Uninstall the ExpoCLI kernel specification"""
    jupyter_data_dir = Path(get_jupyter_data_dir())
    install_dir = jupyter_data_dir / 'kernels' / 'expocli'

    if install_dir.exists():
        shutil.rmtree(install_dir)
        print(f"✓ ExpoCLI kernel uninstalled successfully!")
        print(f"  Removed: {install_dir}")
        return True
    else:
        print(f"✗ ExpoCLI kernel not found at {install_dir}")
        return False


def main():
    """Main installation function"""
    if len(sys.argv) > 1 and sys.argv[1] == '--uninstall':
        return 0 if uninstall_kernelspec() else 1
    else:
        print("Installing ExpoCLI Jupyter Kernel...")
        print("=" * 50)

        # Check if ExpoCLI is installed
        try:
            result = subprocess.run(
                ['expocli', '--version'],
                capture_output=True,
                text=True,
                timeout=2
            )
            print("✓ ExpoCLI found")
        except (subprocess.SubprocessError, FileNotFoundError):
            print("⚠ Warning: ExpoCLI executable not found in PATH")
            print("  The kernel will be installed, but you need ExpoCLI to use it.")
            print("  Install ExpoCLI from: https://github.com/PierreBx/ExpoCLI")
            print()

        # Check if jupyter is installed
        try:
            subprocess.run(['jupyter', '--version'], capture_output=True, check=True)
            print("✓ Jupyter found")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("✗ Error: Jupyter not found. Please install it first:")
            print("  pip install jupyter")
            return 1

        print()
        return 0 if install_kernelspec() else 1


if __name__ == '__main__':
    sys.exit(main())
