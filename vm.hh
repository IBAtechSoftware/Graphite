#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

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
static void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// Represents a single register slot in the VM
// Each VirtualMachineRegister is only identifiable by its register slot
struct VirtualMachineRegister {
  int slot;
  std::string value;

  void writeRegisterValue(std::string v) {
    value = v;

    switch (slot) {
    case 0: // Slot 0: STDOUT with newline
      std::cout << value << std::endl;
    case 1: // Slot 1: STDOUT WITHOUT a newline
      std::cout << value;
    case 2: // Slot 2: STDIN
      std::string input;
      std::cin >> input;
      value = input;
    }
  }

  std::string readRegisterValue() { return value; }
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

  VirtualMachineInstructionResult
  execute(std::vector<VirtualMachineRegister> *registers,
          std::vector<VirtualMachineBuffer> *buffers, std::vector<VirtualMachineBuffer> *tmpBuffers);
};

struct VirtualMachineSector {
  int sectorId;
  std::vector<VirtualMachineInstruction> instructions;

  void execute(std::vector<VirtualMachineRegister> *registers,
               std::vector<VirtualMachineBuffer> *buffers,
               std::vector<VirtualMachineSector> *sectors, std::vector<VirtualMachineBuffer> *tmpBuffers) {
    for (VirtualMachineInstruction instruction : instructions) {
      VirtualMachineInstructionResult result =
          instruction.execute(registers, buffers, tmpBuffers);

      if (result.gotoSector) {
        sectors->at(result.sectorId).execute(registers, buffers, sectors, tmpBuffers);
      }
    }
  }
};

/*
    Statement is structured like:
    $ <-- Indicates that the parser should eval the statement located after the
   $
    [#, @] <-- Indicates what type the statement is, the # symbol is for
   buffer access, the @ symbol is for register access [statement] <-- Register
   or Buffer slot $ <-- End the special statement
*/
static std::string evalSpecialStatement(std::string statement, std::vector<VirtualMachineBuffer> *buffers, std::vector<VirtualMachineRegister> *registers) {

  if (statement[0] == '$') {
    statement.pop_back(); // Remove the last character

    if (statement[1] == '#') {
      statement.erase(0, 2);

      return buffers->at(std::stoi(statement)).value;
    } else if (statement[1] == '@') {
      statement.erase(0, 2);
      return registers->at(std::stoi(statement)).readRegisterValue();
    } else {
      throw std::runtime_error("Invalid special statement modifier");
    }
  } else {
    // Statement cannot be evaluated
    return statement;
  }
}