#!/usr/bin/env python3
"""
ExpoCLI Jupyter Kernel

A Jupyter kernel for executing ExpoCLI SQL-like XML queries in notebooks.
"""

from ipykernel.kernelbase import Kernel
import subprocess
import re
import os
import sys
from typing import Dict, Any, List, Optional


class ExpoCLIKernel(Kernel):
    """Jupyter kernel for ExpoCLI XML query language"""

    implementation = 'ExpoCLI'
    implementation_version = '1.0.0'
    language = 'expocli-sql'
    language_version = '1.0'
    language_info = {
        'name': 'expocli-sql',
        'mimetype': 'text/x-sql',
        'file_extension': '.eql',
        'codemirror_mode': 'sql',
        'pygments_lexer': 'sql'
    }
    banner = """ExpoCLI Kernel - SQL-like XML Querying

Execute SQL-like queries on XML files directly in Jupyter notebooks.

Example queries:
  SELECT name, price FROM examples/books.xml WHERE price > 30
  SELECT * FROM examples/test.xml ORDER BY price DESC

Special commands:
  help                 - Show ExpoCLI help
  SET XSD <file>       - Set XSD schema
  SHOW XSD            - Show current XSD
  GENERATE XML ...    - Generate XML from schema
  CHECK <file>        - Validate XML against schema
"""

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.expocli_path = self._find_expocli()

    def _find_expocli(self) -> str:
        """Locate the ExpoCLI executable"""
        # Try common locations
        candidates = [
            '/usr/local/bin/expocli',
            '/usr/bin/expocli',
            'expocli',  # In PATH
            os.path.expanduser('~/.local/bin/expocli')
        ]

        for path in candidates:
            try:
                result = subprocess.run(
                    [path, '--version'] if path != 'expocli' else ['expocli', '--version'],
                    capture_output=True,
                    text=True,
                    timeout=2
                )
                if result.returncode == 0 or 'expocli' in result.stdout.lower() or 'expocli' in result.stderr.lower():
                    return path
            except (subprocess.SubprocessError, FileNotFoundError):
                continue

        # Default to 'expocli' and let it fail with helpful message if not found
        return 'expocli'

    def _execute_query(self, query: str) -> Dict[str, Any]:
        """
        Execute an ExpoCLI query and return the result

        Returns:
            Dict with 'success', 'output', and 'error' keys
        """
        try:
            # Clean up the query
            query = query.strip()

            if not query:
                return {
                    'success': True,
                    'output': '',
                    'error': None
                }

            # Execute ExpoCLI with the query
            result = subprocess.run(
                [self.expocli_path, query],
                capture_output=True,
                text=True,
                timeout=30,
                cwd=os.getcwd()
            )

            if result.returncode == 0:
                return {
                    'success': True,
                    'output': result.stdout,
                    'error': None
                }
            else:
                return {
                    'success': False,
                    'output': result.stdout,
                    'error': result.stderr or f"ExpoCLI exited with code {result.returncode}"
                }

        except subprocess.TimeoutExpired:
            return {
                'success': False,
                'output': '',
                'error': 'Query execution timed out (30s limit)'
            }
        except FileNotFoundError:
            return {
                'success': False,
                'output': '',
                'error': f'ExpoCLI executable not found at: {self.expocli_path}\n\n'
                        'Please install ExpoCLI first:\n'
                        '  git clone https://github.com/PierreBx/ExpoCLI\n'
                        '  cd ExpoCLI\n'
                        '  ./install.sh'
            }
        except Exception as e:
            return {
                'success': False,
                'output': '',
                'error': f'Unexpected error: {str(e)}'
            }

    def _format_output(self, output: str, is_error: bool = False) -> Dict[str, Any]:
        """
        Format output for Jupyter display

        For POC, we use plain text. Future enhancement: HTML tables.
        """
        if not output:
            return {
                'text/plain': '(no output)'
            }

        # Preserve ANSI colors if present
        return {
            'text/plain': output
        }

    def do_execute(
        self,
        code: str,
        silent: bool,
        store_history: bool = True,
        user_expressions: Optional[Dict] = None,
        allow_stdin: bool = False
    ) -> Dict[str, Any]:
        """
        Execute user code (ExpoCLI query)

        This is the main entry point for code execution in the kernel.
        """
        if not code.strip():
            return {
                'status': 'ok',
                'execution_count': self.execution_count,
                'payload': [],
                'user_expressions': {}
            }

        # Handle magic commands (future enhancement)
        if code.strip().startswith('%'):
            return self._handle_magic(code, silent)

        # Execute the query
        result = self._execute_query(code)

        if not silent:
            if result['success']:
                # Send output to the notebook
                if result['output']:
                    display_data = self._format_output(result['output'])
                    self.send_response(
                        self.iopub_socket,
                        'display_data',
                        {
                            'data': display_data,
                            'metadata': {}
                        }
                    )
            else:
                # Send error
                error_msg = result['error'] or 'Unknown error'
                self.send_response(
                    self.iopub_socket,
                    'stream',
                    {
                        'name': 'stderr',
                        'text': error_msg
                    }
                )

        # Return execution result
        if result['success']:
            return {
                'status': 'ok',
                'execution_count': self.execution_count,
                'payload': [],
                'user_expressions': {}
            }
        else:
            return {
                'status': 'error',
                'execution_count': self.execution_count,
                'ename': 'ExpoCLIError',
                'evalue': result['error'] or 'Query execution failed',
                'traceback': [result['error'] or 'Query execution failed']
            }

    def _handle_magic(self, code: str, silent: bool) -> Dict[str, Any]:
        """
        Handle magic commands (future enhancement)

        For POC, just return a placeholder message.
        """
        magic_name = code.strip().split()[0]

        if not silent:
            self.send_response(
                self.iopub_socket,
                'stream',
                {
                    'name': 'stdout',
                    'text': f'Magic commands not yet implemented in POC: {magic_name}\n'
                           'Future features: %set_xsd, %show_xsd, %export, etc.'
                }
            )

        return {
            'status': 'ok',
            'execution_count': self.execution_count,
            'payload': [],
            'user_expressions': {}
        }

    def do_complete(self, code: str, cursor_pos: int) -> Dict[str, Any]:
        """
        Handle tab completion (future enhancement)
        """
        return {
            'status': 'ok',
            'matches': [],
            'cursor_start': cursor_pos,
            'cursor_end': cursor_pos,
            'metadata': {}
        }

    def do_inspect(self, code: str, cursor_pos: int, detail_level: int = 0) -> Dict[str, Any]:
        """
        Handle introspection (Shift+Tab in Jupyter)
        """
        return {
            'status': 'ok',
            'found': False,
            'data': {},
            'metadata': {}
        }


if __name__ == '__main__':
    from ipykernel.kernelapp import IPKernelApp
    IPKernelApp.launch_instance(kernel_class=ExpoCLIKernel)
