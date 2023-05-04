#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

static std::vector<std::string> split(std::string s, std::string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

// Functions
void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// Represents a single register slot in the VM
// Each VirtualMachineRegister is only identifiable by its register slot
struct VirtualMachineRegister {
   int slot; 
   std::string value;

   void writeRegisterValue(std::string v){
    value = v;

    // TODO: Handle register write
    switch (slot){
        case 0:
            std::cout << value << std::endl;
    }
   }

   std::string readRegisterValue(){
    return value;
   }
};

enum VirtualMachineInstructionType {
    BUFWRITE = 0,
    REGWRITE = 1,
    REGCPYTOBUF = 2,
    BUFCPYTOREG = 3,
    GOTOSECTOR = 4,
    ADD = 5,
    SUB = 6,
    DIV = 7,
    MUL = 8,
    TMPBUFCPY = 9,
    TMPBUFRM = 10
};

struct VirtualMachineInstructionResult {
    bool gotoSector = false;
    int sectorId = -1;
};

struct VirtualMachineBuffer {
    int slot;
    std::string value;
};

struct VirtualMachineInstruction {
    int op;
    std::vector<std::string> params;

    VirtualMachineInstructionResult execute(std::vector<VirtualMachineRegister> *registers, std::vector<VirtualMachineBuffer> *buffers);
};

struct VirtualMachineSector {
    int sectorId;
    std::vector<VirtualMachineInstruction> instructions;

    void execute(std::vector<VirtualMachineRegister> *registers, std::vector<VirtualMachineBuffer> *buffers, std::vector<VirtualMachineSector> *sectors){
        for (VirtualMachineInstruction instruction : instructions){
            VirtualMachineInstructionResult result = instruction.execute(registers, buffers);

            if (result.gotoSector){
                sectors->at(result.sectorId).execute(registers, buffers, sectors);
            }
        }
    }
};