# C++ Development Environment for XML Query CLI
FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    libpugixml-dev \
    gdb \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy project files
COPY . /app

# Build the project (will be done via docker-compose or manually)
# RUN mkdir -p build && cd build && cmake .. && make

CMD ["/bin/bash"]
