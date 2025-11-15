"""
ExpoCLI Jupyter Kernel

A Jupyter kernel for executing ExpoCLI SQL-like XML queries in notebooks.
"""

__version__ = '1.0.0'

from .kernel import ExpoCLIKernel

__all__ = ['ExpoCLIKernel']
