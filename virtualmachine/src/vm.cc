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
    buffer.value = evalSpecialStatement(params[1], buffers, registers);

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
    buffer.slot = tmpBuffers->capacity();
    buffer.value = std::to_string(num1 + num2);

    tmpBuffers->push_back(
        buffer); // Write the new temporary buffer into garbage collected memory
  } else if (op == VirtualMachineInstructionType::SUB) {
    int num1 = std::stoi(evalSpecialStatement(params[0], buffers, registers));
    int num2 = std::stoi(evalSpecialStatement(params[1], buffers, registers));

    VirtualMachineBuffer buffer{};
    buffer.slot = tmpBuffers->capacity();
    buffer.value = std::to_string(num1 - num2);

    tmpBuffers->push_back(
        buffer); // Write the new temporary buffer into garbage collected memory
  } else if (op == VirtualMachineInstructionType::MUL) {
    int num1 = std::stoi(evalSpecialStatement(params[0], buffers, registers));
    int num2 = std::stoi(evalSpecialStatement(params[1], buffers, registers));

    VirtualMachineBuffer buffer{};
    buffer.slot = tmpBuffers->capacity();
    buffer.value = std::to_string(num1 * num2);

    tmpBuffers->push_back(
        buffer); // Write the new temporary buffer into garbage collected memory
  } else if (op == VirtualMachineInstructionType::DIV) {
    int num1 = std::stoi(evalSpecialStatement(params[0], buffers, registers));
    int num2 = std::stoi(evalSpecialStatement(params[1], buffers, registers));

    VirtualMachineBuffer buffer{};
    buffer.slot = tmpBuffers->capacity();
    buffer.value = std::to_string(num1 / num2);

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
  } else if (op == VirtualMachineInstructionType::TMPBUFRM) {
    if (params[0] == "_last_") {
      params[0] = std::to_string(tmpBuffers->size() - 1);
    }

    int tmpBufferSlot =
        std::stoi(evalSpecialStatement(params[0], buffers, registers));

    tmpBuffers->erase(tmpBuffers->begin() + tmpBufferSlot);
  } else if (op == VirtualMachineInstructionType::WABWRITE) {
    VirtualMachineInstructionResult result{};
    result.gotoSector = false;
    result.writeAheadToBuffer = true;
    result.writeAheadBufferContent = params[2];
    result.writeAheadBufferId = std::stoi(params[1]);
    result.sectorId = std::stoi(params[0]);

    return result;
  } else if (op == WABCPYTOBUF){

    int bufferCopyId = 0;

    if (params[1] == "_new_") {
      bufferCopyId = -1;
    } else {
      bufferCopyId = std::stoi(params[1]);
    }

    VirtualMachineInstructionResult result{};
    result.writeAheadBufferCopy = true;
    result.writeAheadBufferId = std::stoi(params[0]);
    result.writeAheadBufferCopyId = bufferCopyId;

    return result; 
  } else if (op == BUFRM){
    int slot = std::stoi(params[0]);

    for (int i = 0; i < buffers->size(); i++){
      VirtualMachineBuffer buffer = buffers->at(i);

      if (buffer.slot == slot){
        buffers->erase(buffers->begin()+buffer.slot);
        break;
      }
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
  } else if (name == "WABWRITE") {
    return WABWRITE;
  } else if (name == "WABRM") {
    return WABRM;
  } else if (name == "WABCPYTOBUF") {
    return WABCPYTOBUF;
  } else if (name == "BUFRM") {
    return BUFRM;
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
  std::string virtualMachineBytecodeFile = "";

  if (argc == 1) {
    std::cout << "Graphite virtual machine v1.0" << std::endl;
    std::cout << "Author: Interfiber <webmaster@interfiber.dev>" << std::endl;
    std::cout << "Syntax: " << argv[0] << " FILE OPTIONS" << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "--virtual-machine-enable-debug-output  Enable debug output "
                 "at the end of program execution"
              << std::endl;
    std::exit(-1);
  } else {
    virtualMachineBytecodeFile = std::string(argv[1]);
  }

  for (int i = 1; i < argc; i++) {
    std::string arg = std::string(argv[i]);

    if (arg == "--virtual-machine-enable-debug-output") {
      virtualMachineDebugOutput = true;
    }
  }

  std::vector<VirtualMachineSector> sectors; // Program sectors
  std::vector<VirtualMachineBuffer> buffers; // Program memory
  std::vector<VirtualMachineBuffer>
      tmpBuffers; // Temporary machine memory, program copies result from these
                  // into program memory
  std::vector<VirtualMachineRegister> registers; // Machine memory

  registers.push_back(VirtualMachineRegister{0});
  registers.push_back(VirtualMachineRegister{1});
  registers.push_back(VirtualMachineRegister{2});

  std::ifstream ifs(virtualMachineBytecodeFile);

  std::string line;

  // Load all sectors into memory
  bool inSector = false;
  VirtualMachineSector sector;
  int sectorId = 0;
  while (std::getline(ifs, line)) {
    ltrim(line); // Remove indents, if they exist

    if (line == "#-#" && !inSector) {
      sector = VirtualMachineSector{sectorId, {}, {}};
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

  int totalInstructionCount = 0;

  for (auto sector : sectors) {
    totalInstructionCount += sector.instructions.size();
  }

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

    TextTable tt('-', '|', '+');
    tt.add("slot");
    tt.add("value");
    tt.endOfRow();

    for (auto tmpBuffer : tmpBuffers) {
      tt.add(std::to_string(tmpBuffer.slot));
      tt.add(tmpBuffer.value);
      tt.endOfRow();
    }

    tt.setAlignment(2, TextTable::Alignment::RIGHT);

    std::cout << "--- TEMPORARY BUFFER MEMORY ---" << std::endl;
    std::cout << tt;
  }
}