#pragma once
#include <string>

struct GraphiteCompilerFunction {
    int id; // AKA the sector slot id for the vm
    std::string bytecodeContent;
};