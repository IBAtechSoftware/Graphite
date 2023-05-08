buildVm:
	mkdir -p bin
	g++ ./virtualmachine/src/vm.cc -g -o ./bin/grvm