#!/bin/bash

# Quick build and test script
# This will build both examples and show usage instructions

set -e

mkdir -p build
cd build

echo "📦 Configuring with CMake (this will download llama.cpp)..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS

echo "⚙️  Compiling examples..."
make -j$(nproc)
