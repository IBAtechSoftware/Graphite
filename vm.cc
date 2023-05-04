#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

std::vector<std::string> split(std::string s, std::string delimiter) {
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

struct VirtualMachineBuffer {
    int slot;
    std::string value;
};


// Represents a single instruction given to the VM
struct VirtualMachineInstruction {
    int op;
    std::vector<std::string> params;

    void execute(std::vector<VirtualMachineRegister> *registers, std::vector<VirtualMachineBuffer> *buffers){
        if (op == VirtualMachineInstructionType::BUFWRITE){

            int slot = std::stoi(params[0]);

            VirtualMachineBuffer buffer{};
            buffer.slot = slot;
            buffer.value = params[1];

            buffers.push_back(buffer);

            std::cout << "Wrote new buffer into memory" << std::endl;
        } else if (op == VirtualMachineInstructionType::REGWRITE){
            int slot = std::stoi(params[0]);

            registers.at(slot).value = params[1];

            std::cout << "Wrote register value" << std::endl;
        } else if (op == VirtualMachineInstructionType::REGCPYTOBUF){
            int registerSlot = std::stoi(params[0]);
            int bufferSlot = std::stoi(params[1]);

            // Copy the contents of the register into the buffer
            // Overwrites the contents of the buffer, with the value of the register

            // Check if the buffer slot exists, if not, create it
            if (bufferSlot >= buffers.size()){
                VirtualMachineBuffer buffer{};
                buffer.slot = buffers.size() + 1;
                buffer.value = registers.at(registerSlot).value;

                buffers.push_back(buffer);
            } else {
                buffers.at(bufferSlot).value = registers.at(registerSlot).value;
            }

            std::cout << "Copied register value to buffer" << std::endl;
        }
    }
};

struct VirtualMachineSector {
    int sectorId;
    std::vector<VirtualMachineInstruction> instructions;

    void execute(std::vector<VirtualMachineRegister> *registers, std::vector<VirtualMachineBuffer> *buffers){
        for (VirtualMachineInstruction instruction : instructions){
            instruction.execute(registers, buffers);
        }
    }
};

VirtualMachineInstructionType instructionNameToType(std::string name){
    if (name == "BUFWRITE"){
        return BUFWRITE;
    } else if (name == "REGWRITE"){
        return REGWRITE;
    } else if (name == "REGCPYTOBUF"){
        return REGCPYTOBUF;
    } else if (name == "BUFCPYTOREG"){
        return BUFCPYTOREG;
    } else if (name == "GOTOSECTOR"){
        return GOTOSECTOR;
    } else if (name == "ADD"){
        return ADD;
    } else if (name == "SUB"){
        return SUB;
    } else if (name == "DIV"){
        return DIV;
    } else if (name == "MUL"){
        return MUL;
    } else if (name == "TMPBUFCPY"){
        return TMPBUFCPY;
    } else if (name == "TMPBUFRM"){
        return TMPBUFRM;
    } else {
        throw std::runtime_error("Invalid instruction name: " + name);
    }
}

VirtualMachineInstruction parseInstruction(std::string instructionLine){
   std::vector<std::string> instructionSyntaxParsed = split(instructionLine, "-");
   std::string instructionName = instructionSyntaxParsed[0];
   std::string instructionArgs = instructionSyntaxParsed[1];
   std::vector<std::string> instructionArgsParsed = split(instructionArgs, ",");

   VirtualMachineInstruction instruction{};
   instruction.params = instructionArgsParsed;
   instruction.op = instructionNameToType(instructionName);

   return instruction;
}

int main(){
   std::vector<VirtualMachineSector> sectors;  // Program sectors
   std::vector<VirtualMachineBuffer> buffers; // Program memory
   std::vector<VirtualMachineRegister> registers; // Machine memory

   std::cout << "Creating VM registers" << std::endl;

    registers.push_back(VirtualMachineRegister {
        0
    });


    std::cout << "Loading bytecode from disk" << std::endl;

    std::ifstream ifs("main.grbc");

    std::string line;

    std::cout << "Compiling bytecode into sectors" << std::endl;

    // Load all sectors into memory
    bool inSector = false;
    VirtualMachineSector sector;
    int sectorId = 0;
    while (std::getline(ifs, line)){
        ltrim(line); // Remove indents, if they exist

        if (line == "#-#" && !inSector) {
            sector = VirtualMachineSector {
                sectorId,
                {}
            };
            inSector = true;

        } else if (line == "#-#" && inSector){
            inSector = false;
            sectorId += 1;

            sectors.push_back(sector);
        } else if (inSector){
            // Parse instruction
            VirtualMachineInstruction ins = parseInstruction(line);

            sector.instructions.push_back(ins);
        }
    }

    std::cout << "Compiled application, executing" << std::endl;

    int totalInstructionCount = 0;
    
    for (auto sector : sectors){
        totalInstructionCount += sector.instructions.size();
    }

    std::cout << "---------------- PROGRAM OUTPUT ----------------" << std::endl;
    std::cout << "VirtualMachineDebug:" << std::endl;
    std::cout << "Graphite Language Virtual Machine v1.0" << std::endl;
    std::cout << "Found a total of " << totalInstructionCount << " VM instructions" << std::endl;
    std::cout << "Found a total of " << sectors.size() << " program sectors" << std::endl;

    // Execute sector 0

    sectors.at(0).execute(registers, buffers);
}
