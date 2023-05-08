buildVm:
	mkdir -p bin
	g++ ./virtualmachine/src/vm.cc -g -o ./bin/grvm

buildCompiler:
	g++ ./compiler/compiler.cc -g -o ./bin/grc

all: buildVm buildCompiler