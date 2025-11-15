# ExpoCLI Jupyter Kernel

Query XML files with SQL-like syntax directly in Jupyter notebooks!

## Overview

The ExpoCLI Jupyter kernel allows you to execute ExpoCLI queries in Jupyter notebooks, combining the power of SQL-like XML querying with the interactive notebook environment.

**Status:** Proof of Concept (POC) - Basic functionality implemented

## Features

✅ **Implemented in POC:**
- Execute ExpoCLI queries in notebook cells
- View query results inline
- Mix queries with Markdown documentation
- Error handling and display
- Support for all ExpoCLI query syntax (SELECT, WHERE, ORDER BY, GROUP BY, etc.)
- Special commands (help, SET XSD, etc.)

🚧 **Planned for Future:**
- Rich HTML table formatting
- Magic commands (`%set_xsd`, `%export`, etc.)
- Tab completion
- Syntax highlighting in cells
- Query result caching
- Integration with pandas DataFrames
- Built-in visualization helpers

## Installation

### Option 1: Docker Installation (Recommended)

The easiest way to get started is using Docker, which includes everything pre-configured.

**1. Build and start the Jupyter service:**

```bash
# Build the Docker image (includes ExpoCLI, Jupyter, and the kernel)
docker compose build

# Start the Jupyter service
docker compose up -d jupyter
```

**2. Access Jupyter Lab:**

Open your browser and navigate to:
```
http://localhost:8888
```

**3. Try the demo notebook:**

Navigate to `examples/ExpoCLI_Demo.ipynb` in the Jupyter Lab interface.

**4. Stop Jupyter when done:**

```bash
docker compose down
```

**Benefits of Docker approach:**
- ✅ No local Python/Jupyter installation required
- ✅ Consistent environment across all platforms
- ✅ Everything pre-configured and ready to use
- ✅ Isolated from your host system

### Option 2: Host Installation

If you prefer to run Jupyter on your host system:

#### Prerequisites

1. **ExpoCLI CLI** - Must be installed first
   ```bash
   git clone https://github.com/PierreBx/ExpoCLI
   cd ExpoCLI
   ./install.sh
   ```

2. **Python 3.8+** and **Jupyter**
   ```bash
   # Check Python version
   python3 --version

   # Install Jupyter if not already installed
   pip3 install jupyter jupyterlab
   ```

#### Install the Kernel

From the ExpoCLI repository root:

```bash
# Run the installation script
./install_kernel.sh

# Verify installation
jupyter kernelspec list
```

You should see `expocli` in the list of available kernels.

#### Manual Installation (Alternative)

If the script doesn't work, you can install manually:

```bash
# Install dependencies
pip3 install --user ipykernel jupyter-client

# Install the kernel package
pip3 install --user -e .

# Install the kernelspec
python3 -m expocli_kernel.install
```

## Quick Start

### Docker Quick Start (Recommended)

```bash
# 1. Start Jupyter Lab in Docker
docker compose up -d jupyter

# 2. Open browser to http://localhost:8888

# 3. Open examples/ExpoCLI_Demo.ipynb and run the cells
```

That's it! Everything is pre-configured.

### Host Installation Quick Start

**1. Start Jupyter**

```bash
# Option 1: Jupyter Notebook (classic interface)
jupyter notebook

# Option 2: JupyterLab (modern interface)
jupyter lab
```

**2. Create a New Notebook**

- Click "New" → "ExpoCLI" (in Jupyter Notebook)
- Or select "ExpoCLI" kernel when creating a new notebook (in JupyterLab)

**3. Run Your First Query**

In a cell, type:

```sql
SELECT * FROM examples/test.xml
```

Press `Shift+Enter` to execute.

**4. Try the Demo Notebook**

Open `examples/ExpoCLI_Demo.ipynb` for a comprehensive tutorial with example queries.

## Usage Examples

### Basic Query

```sql
SELECT name, price FROM examples/books.xml WHERE price > 30
```

### Multi-line Queries

```sql
SELECT title, author, price
FROM examples/books.xml
WHERE @category = 'web'
ORDER BY price DESC
```

### Querying Multiple Files

```sql
SELECT FILE_NAME, title, price
FROM examples/bookstore_*.xml
ORDER BY price DESC
```

### Aggregation

```sql
SELECT @category, COUNT(*), AVG(price)
FROM examples/books.xml
GROUP BY @category
```

### Special Commands

```sql
help
```

```sql
SET XSD examples/bookstore_schema.xsd
```

```sql
SHOW XSD
```

## Mixing with Python (Future Enhancement)

Once pandas integration is added, you'll be able to do things like:

```python
# In a Python cell
import pandas as pd
import matplotlib.pyplot as plt

# Future magic command to convert last result to pandas
# %to_pandas results
# results.plot(kind='bar')
```

## Architecture

```
┌─────────────────────────────────┐
│  Jupyter Notebook (Browser UI)  │
└────────────┬────────────────────┘
             │ WebSocket
┌────────────┴────────────────────┐
│      Jupyter Server (Python)     │
└────────────┬────────────────────┘
             │ ZMQ Messages
┌────────────┴────────────────────┐
│   ExpoCLI Kernel (Python)        │
│   - Implements kernel protocol   │
│   - Formats output               │
└────────────┬────────────────────┘
             │ subprocess
┌────────────┴────────────────────┐
│   ExpoCLI CLI (C++ binary)       │
│   - Unchanged from CLI version   │
└──────────────────────────────────┘
```

The kernel is a lightweight Python wrapper that:
1. Receives query code from Jupyter
2. Executes ExpoCLI as a subprocess
3. Captures and formats the output
4. Sends results back to Jupyter for display

## Project Structure

```
ExpoCLI/
├── expocli_kernel/              # Kernel package
│   ├── __init__.py             # Package initialization
│   ├── __main__.py             # Entry point for kernel
│   ├── kernel.py               # Main kernel implementation
│   ├── install.py              # Kernelspec installer
│   └── kernelspec/             # Kernel specification
│       └── kernel.json         # Kernel configuration
├── examples/
│   └── ExpoCLI_Demo.ipynb      # Demo notebook
├── setup.py                    # Python package setup
├── install_kernel.sh           # Installation script
└── JUPYTER_KERNEL.md           # This file
```

## Troubleshooting

### Kernel not showing up

```bash
# List installed kernels
jupyter kernelspec list

# If not there, reinstall
./install_kernel.sh
```

### ExpoCLI executable not found

The kernel looks for `expocli` in these locations:
- `/usr/local/bin/expocli`
- `/usr/bin/expocli`
- `~/.local/bin/expocli`
- In your PATH

Make sure ExpoCLI CLI is installed and accessible.

### Queries time out

By default, queries have a 30-second timeout. For longer queries, you may need to modify `expocli_kernel/kernel.py` and increase the timeout parameter.

### Import errors

```bash
# Reinstall dependencies
pip3 install --user --upgrade ipykernel jupyter-client

# Reinstall kernel package
pip3 install --user --force-reinstall -e .
```

### Jupyter server issues

```bash
# Restart Jupyter
# Press Ctrl+C in the terminal running Jupyter, then start again
jupyter notebook
```

## Uninstallation

```bash
# Uninstall the kernelspec
./install_kernel.sh --uninstall

# Or manually
python3 -m expocli_kernel.install --uninstall

# Optionally, remove the Python package
pip3 uninstall expocli-kernel
```

## Development

### Running in Development Mode

The kernel is installed in "editable" mode (`pip install -e .`), so changes to the kernel code take effect after restarting the Jupyter kernel (Kernel → Restart in the notebook menu).

### Testing Changes

1. Make changes to `expocli_kernel/kernel.py`
2. In your notebook: Kernel → Restart
3. Test your changes

### Adding Features

Key areas for enhancement:

**1. Rich Output Formatting** (`kernel.py:_format_output()`)
- Parse table output and convert to HTML
- Add CSS styling

**2. Magic Commands** (`kernel.py:_handle_magic()`)
- `%set_xsd <file>` - Set XSD schema
- `%show_xsd` - Display current schema
- `%export <file>` - Save last result to file
- `%to_pandas` - Convert to DataFrame

**3. Tab Completion** (`kernel.py:do_complete()`)
- Suggest field names from XML
- Complete file paths

**4. Syntax Highlighting**
- Create custom CodeMirror mode for ExpoCLI syntax
- Add to `kernelspec/kernel.json`

## Contributing

Contributions welcome! Areas for improvement:

- [ ] HTML table rendering with CSS
- [ ] Pandas DataFrame integration
- [ ] Visualization helpers
- [ ] Better error messages
- [ ] Progress indicators for long queries
- [ ] Query result caching
- [ ] Syntax highlighting mode
- [ ] Tab completion
- [ ] Kernel logo/icon

## Resources

- [ExpoCLI Main Documentation](README.md)
- [Jupyter Kernel Documentation](https://jupyter-client.readthedocs.io/en/stable/kernels.html)
- [Jupyter Integration Analysis](JUPYTER_INTEGRATION_ANALYSIS.md)

## License

Same as ExpoCLI main project.

## Support

- GitHub Issues: https://github.com/PierreBx/ExpoCLI/issues
- Main README: [README.md](README.md)

---

**Status Update:** This is a Proof of Concept implementation. Basic functionality is working. Future enhancements will add rich formatting, magic commands, and better integration with the Python ecosystem.
