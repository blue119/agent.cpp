# Shell Execution with Human-In-The-Loop

Instead of requiring separate tools for every operation, an agent can combine multiple operations into a single shell command or script.

## Building Blocks

### Tools

This example exposes a single tool that can execute the given shell commands:

- **shell**: Execute shell commands or scripts.

### Callbacks


## Building

```bash
cd examples/shell

git -C ../.. submodule update --init --recursive

cmake -B build
cmake --build build -j$(nproc)
```

### Using a custom llama.cpp

If you have llama.cpp already downloaded:

```bash
cmake -B build -DLLAMA_CPP_DIR=/path/to/your/llama.cpp
cmake --build build -j$(nproc)
```

## Usage

```bash
./shell-example -m <path-to-model.gguf> [-d <working-directory>]
```

Options:
- `-m <path>` - Path to the GGUF model file (required)
- `-d <path>` - Working directory for shell commands (default: current directory)

## Example

Start a conversation and ask for something that requires multiple commands to run.

```console
$ ./build/shell-example -m ../../granite-4.0-micro-Q8_0.gguf
> Find all .cpp files in this directory and count their lines
<tool_call>
{"name": "shell", "arguments": {
  "command": "for f in *.cpp; do echo \"Processing $f\"; wc -l < \"$f\"; done"
}}
</tool_call>
[TOOL EXECUTION] Executing 1 tool call(s)
[TOOL RESULT]
{"output":"Processing shell-example.cpp\n198\n"}
The directory contains one `.cpp` file: **shell-example.cpp**.
It has **198 lines**.
```

## Security Considerations

- Commands are executed with the permissions of the running process
- Consider running in a sandboxed environment or container for untrusted inputs
