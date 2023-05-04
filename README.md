# Graphite
Language Virtual Machine which can run graphite bytecode

## Key concepts

### Registers
Registers are a way for a program to access virtual machine APIs, such as the filesystem, stdout, stdin, networking, etc. Registers can be written to with the ```REGWRITE``` command, providing the register slot, and value to write

### Buffers
Buffers are a way for a program to keep track of data. Buffers are program managed by the program, and can be created with the ```BUFWRITE``` command.