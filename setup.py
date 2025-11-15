#!/usr/bin/env python3
"""
Setup script for ExpoCLI Jupyter Kernel
"""

from setuptools import setup, find_packages
import os

# Read version from package
version = {}
with open(os.path.join("expocli_kernel", "__init__.py")) as f:
    exec([line for line in f if line.startswith("__version__")][0], version)

setup(
    name='expocli-kernel',
    version=version['__version__'],
    description='Jupyter kernel for ExpoCLI - SQL-like XML querying',
    long_description=open('README.md').read() if os.path.exists('README.md') else '',
    long_description_content_type='text/markdown',
    author='ExpoCLI Contributors',
    url='https://github.com/PierreBx/ExpoCLI',
    packages=find_packages(),
    package_data={
        'expocli_kernel': ['kernelspec/kernel.json']
    },
    install_requires=[
        'ipykernel>=6.0.0',
        'jupyter-client>=7.0.0'
    ],
    python_requires='>=3.8',
    classifiers=[
        'Framework :: Jupyter',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
    ],
    entry_points={
        'console_scripts': [
            'expocli-kernel-install=expocli_kernel.install:main',
        ],
    },
)
