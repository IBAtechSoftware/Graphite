#include "vm.hh"
#include "txttable.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Represents a single instruction given to the VM
VirtualMachineInstructionResult VirtualMachineInstruction::execute(
    std::vector<VirtualMachineRegister> *registers,
    std::vector<VirtualMachineBuffer> *buffers,
    std::vector<VirtualMachineBuffer> *tmpBuffers) {
  if (op == VirtualMachineInstructionType::BUFWRITE) {

    int slot = std::stoi(evalSpecialStatement(params[0], buffers, registers));

    VirtualMachineBuffer buffer{};
    buffer.slot = slot;
    buffer.value = params[1];

    buffers->push_back(buffer);
  } else if (op == VirtualMachineInstructionType::REGWRITE) {
    int slot = std::stoi(evalSpecialStatement(params[0], buffers, registers));

    registers->at(slot).writeRegisterValue(
        evalSpecialStatement(params[1], buffers, registers));
  } else if (op == VirtualMachineInstructionType::REGCPYTOBUF) {
    int registerSlot =
        std::stoi(evalSpecialStatement(params[0], buffers, registers));
    int bufferSlot =
        std::stoi(evalSpecialStatement(params[1], buffers, registers));

    // Copy the contents of the register into the buffer
    // Overwrites the contents of the buffer, with the value of the register

    // Check if the buffer slot exists, if not, create it
    if (bufferSlot >= buffers->size()) {
      VirtualMachineBuffer buffer{};
      buffer.slot = bufferSlot;
      buffer.value = registers->at(registerSlot).readRegisterValue();

      buffers->push_back(buffer);
    } else {
      buffers->at(bufferSlot).value = registers->at(registerSlot).value;
    }
  } else if (op == VirtualMachineInstructionType::BUFCPYTOREG) {
    int bufferSlot =
        std::stoi(evalSpecialStatement(params[0], buffers, registers));
    int registerSlot =
        std::stoi(evalSpecialStatement(params[1], buffers, registers));

    if (bufferSlot >= buffers->size()) {
      throw std::runtime_error("Invalid buffer slot for copy");
    }

    registers->at(registerSlot)
        .writeRegisterValue(buffers->at(bufferSlot).value);
  } else if (op == VirtualMachineInstructionType::GOTOSECTOR) {
    VirtualMachineInstructionResult result{};
    result.gotoSector = true;
    result.sectorId =
        std::stoi(evalSpecialStatement(params[0], buffers, registers));

    return result;
  } else if (op == VirtualMachineInstructionType::ADD) {
    int num1 = std::stoi(evalSpecialStatement(params[0], buffers, registers));
    int num2 = std::stoi(evalSpecialStatement(params[1], buffers, registers));

    VirtualMachineBuffer buffer{};
    buffer.slot = tmpBuffers->size() - 1;
    buffer.value = std::to_string(num1 + num2);

    tmpBuffers->push_back(
        buffer); // Write the new temporary buffer into garbage collected memory
  } else if (op == VirtualMachineInstructionType::TMPBUFCPY) {

    // _last_ transforms into the slot of the last buffer inserted into
    // temporary memory
    if (params[0] == "_last_") {
      params[0] = std::to_string(tmpBuffers->size() - 1);
    }

    int tmpBufSlot = std::stoi(params[0]);
    int bufferSlot = std::stoi(params[1]);

    if (bufferSlot >= buffers->size()) {
      // The buffer does not exist, create it
      VirtualMachineBuffer targetBuffer{};
      targetBuffer.slot = bufferSlot;
      targetBuffer.value = tmpBuffers->at(tmpBufSlot).value;

      buffers->push_back(targetBuffer);
    } else {
      // Buffer exists, overwrite the value
      buffers->at(bufferSlot).value = tmpBuffers->at(tmpBufSlot).value;
    }
  }

  VirtualMachineInstructionResult result{};
  result.gotoSector = false;

  return result;
}

VirtualMachineInstructionType instructionNameToType(std::string name) {
  if (name == "BUFWRITE") {
    return BUFWRITE;
  } else if (name == "REGWRITE") {
    return REGWRITE;
  } else if (name == "REGCPYTOBUF") {
    return REGCPYTOBUF;
  } else if (name == "BUFCPYTOREG") {
    return BUFCPYTOREG;
  } else if (name == "GOTOSECTOR") {
    return GOTOSECTOR;
  } else if (name == "ADD") {
    return ADD;
  } else if (name == "SUB") {
    return SUB;
  } else if (name == "DIV") {
    return DIV;
  } else if (name == "MUL") {
    return MUL;
  } else if (name == "TMPBUFCPY") {
    return TMPBUFCPY;
  } else if (name == "TMPBUFRM") {
    return TMPBUFRM;
  } else {
    throw std::runtime_error("Invalid instruction name: " + name);
  }
}

VirtualMachineInstruction parseInstruction(std::string instructionLine) {
  std::vector<std::string> instructionSyntaxParsed =
      split(instructionLine, "-");
  std::string instructionName = instructionSyntaxParsed[0];
  std::string instructionArgs = instructionSyntaxParsed[1];
  std::vector<std::string> instructionArgsParsed = split(instructionArgs, ",");

  VirtualMachineInstruction instruction{};
  instruction.params = instructionArgsParsed;
  instruction.op = instructionNameToType(instructionName);

  return instruction;
}

int main(int argc, char *argv[]) {

  bool virtualMachineDebugOutput = false;

  for (int i = 0; i < argc; i++){
    std::string arg = std::string(argv[i]);

    if (arg == "--virtual-machine-enable-debug-output"){
      virtualMachineDebugOutput = true;
    }
  }

  std::vector<VirtualMachineSector> sectors; // Program sectors
  std::vector<VirtualMachineBuffer> buffers; // Program memory
  std::vector<VirtualMachineBuffer>
      tmpBuffers; // Temporary machine memory, program copies result from these
                  // into program memory
  std::vector<VirtualMachineRegister> registers; // Machine memory

  std::cout << "Creating VM registers" << std::endl;

  registers.push_back(VirtualMachineRegister{0});

  std::cout << "Loading bytecode from disk" << std::endl;

  std::ifstream ifs("main.grbc");

  std::string line;

  std::cout << "Compiling bytecode into sectors" << std::endl;

  // Load all sectors into memory
  bool inSector = false;
  VirtualMachineSector sector;
  int sectorId = 0;
  while (std::getline(ifs, line)) {
    ltrim(line); // Remove indents, if they exist

    if (line == "#-#" && !inSector) {
      sector = VirtualMachineSector{sectorId, {}};
      inSector = true;

    } else if (line == "#-#" && inSector) {
      inSector = false;
      sectorId += 1;

      sectors.push_back(sector);
    } else if (inSector) {
      // Parse instruction
      VirtualMachineInstruction ins = parseInstruction(line);

      sector.instructions.push_back(ins);
    }
  }

  std::cout << "Compiled application, executing" << std::endl;

  int totalInstructionCount = 0;

  for (auto sector : sectors) {
    totalInstructionCount += sector.instructions.size();
  }

  std::cout << "---------------- PROGRAM OUTPUT ----------------" << std::endl;
  //  std::cout << "VirtualMachineDebug:" << std::endl;
  //  std::cout << "Graphite Language Virtual Machine v1.0" << std::endl;
  //  std::cout << "Found a total of " << totalInstructionCount
  //            << " VM instructions" << std::endl;
  // std::cout << "Found a total of " << sectors.size() << " program sectors"
  // << std::endl;

  // Execute sector 0

  sectors.at(0).execute(&registers, &buffers, &sectors, &tmpBuffers);

  if (virtualMachineDebugOutput) {
    // Render tables
    std::cout << "---------------- PROGRAM RESULT ----------------"
              << std::endl;

    std::cout << "--- BUFFERS ---" << std::endl;
    TextTable t('-', '|', '+');
    t.add("Slot");
    t.add("Value");
    t.endOfRow();

    for (auto buffer : buffers) {
      t.add(std::to_string(buffer.slot));
      t.add(buffer.value);
      t.endOfRow();
    }

    t.setAlignment(2, TextTable::Alignment::RIGHT);
    std::cout << t;

    TextTable rt('-', '|', '+');

    rt.add("slot");
    rt.add("value");
    rt.endOfRow();

    for (auto mRegister : registers) {
      rt.add(std::to_string(mRegister.slot));
      rt.add(mRegister.value);
      rt.endOfRow();
    }
    rt.setAlignment(2, TextTable::Alignment::RIGHT);

    std::cout << "--- REGISTERS ---" << std::endl;
    std::cout << rt;
  }
}
