# expocli Installation Guide

This guide explains how to use expocli transparently on your local machine while it runs inside a Docker container.

## Quick Install (Recommended)

Run the automated installer:

```bash
cd /path/to/ExpoCLI
./install.sh
```

Then reload your shell configuration:

```bash
source ~/.bashrc  # or ~/.zshrc for zsh users
```

That's it! You can now use `expocli` from anywhere.

## What the Installer Does

1. **Creates an alias** in your shell configuration (`~/.bashrc` or `~/.zshrc`)
2. **Links to the wrapper script** that handles all Docker operations transparently
3. **Tests the setup** to ensure everything works

## How It Works

The `expocli.sh` wrapper script automatically:

✅ **Checks** if Docker image exists (builds if needed)
✅ **Checks** if expocli binary exists (compiles if needed)
✅ **Mounts** your current directory into the container
✅ **Passes** all arguments transparently
✅ **Handles** interactive mode (REPL) and single queries
✅ **Preserves** exit codes for scripting

**You won't even notice it's running in a container!**

## Usage Examples

### Interactive Mode (REPL)
```bash
expocli
# You'll see: expocli>
```

### Single Query Mode
```bash
expocli "SELECT breakfast_menu/food/name FROM ./examples WHERE breakfast_menu/food/calories < 500"
```

### Query Local Files
```bash
cd /path/to/your/xml/files
expocli "SELECT * FROM ."
```

The wrapper automatically mounts your current directory, so you can query files from anywhere on your system!

### Use in Scripts
```bash
#!/bin/bash
result=$(expocli "SELECT name FROM ./data WHERE price < 100")
echo "Found: $result"
```

## First Run

The first time you run `expocli`, it will:
1. Build the Docker image (~1 minute)
2. Compile the expocli binary (~30 seconds)

**Subsequent runs are instant!**

## Manual Setup (Alternative)

If you prefer not to use the installer, you can manually add this to your `~/.bashrc` or `~/.zshrc`:

```bash
alias expocli='/full/path/to/ExpoCLI/expocli.sh'
```

## Updating expocli

If you make code changes and want to rebuild:

```bash
# Just delete the binary - the wrapper will rebuild automatically
docker compose run --rm xml-query-cli rm /app/build/expocli
expocli  # Will trigger rebuild
```

Or rebuild manually:

```bash
cd /path/to/ExpoCLI
docker compose run --rm xml-query-cli bash -c "cd /app/build && make"
```

## Uninstalling

To remove expocli:

```bash
# Remove the alias from your shell config
sed -i.bak '/alias expocli=/d' ~/.bashrc  # or ~/.zshrc

# Remove Docker image (optional)
cd /path/to/ExpoCLI
docker compose down --rmi all
```

## Troubleshooting

### "Docker is not installed"
Install Docker from: https://docs.docker.com/get-docker/

### "Docker Compose V2 is not available"
Update Docker to the latest version. Docker Compose V2 is built into modern Docker.

### Wrapper script is slow
The first run builds everything. Subsequent runs should be instant. If still slow:
- Check Docker is running: `docker ps`
- Check image exists: `docker images | grep expocli`

### "Permission denied" errors
Ensure the wrapper script is executable:
```bash
chmod +x /path/to/ExpoCLI/expocli.sh
```

### File path issues
The wrapper mounts your **current directory** as `/workspace` inside the container.
Use relative paths (e.g., `./data`) or absolute paths from your current directory.

Example:
```bash
cd /home/user/myproject
expocli "SELECT name FROM ./xmlfiles"  # ✅ Works
expocli "SELECT name FROM /workspace/xmlfiles"  # ✅ Also works (same thing)
```

## Advanced Usage

### Disable Progress Messages
```bash
EXPOCLI_QUIET=1 expocli "SELECT ..."
```

### Run Without Alias
```bash
/path/to/ExpoCLI/expocli.sh "SELECT ..."
```

### Access Container Directly
```bash
cd /path/to/ExpoCLI
docker compose run --rm xml-query-cli bash
```

## Benefits of This Approach

✅ **Portable** - Works on Linux, macOS, and Windows (with WSL)
✅ **Consistent** - Same environment everywhere via Docker
✅ **Transparent** - Feels like a native command
✅ **Automatic** - Builds and compiles when needed
✅ **Clean** - No local dependencies to install
✅ **Flexible** - Can still access container directly if needed

---

For more information, see the main [README.md](README.md).
