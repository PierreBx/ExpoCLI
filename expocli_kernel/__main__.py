"""
Entry point for running the ExpoCLI kernel
"""

if __name__ == '__main__':
    from ipykernel.kernelapp import IPKernelApp
    from .kernel import ExpoCLIKernel

    IPKernelApp.launch_instance(kernel_class=ExpoCLIKernel)
