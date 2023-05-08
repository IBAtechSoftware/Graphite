#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>

std::string slurp(std::ifstream& in) {
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

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
static void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}



/*
    Graphite LISP compiler
    Default compiler exposed functions:
    regwrite
    regcpytobuf
    etc(all of the graphite bytecode commands)
*/


int main(){
    std::string compilerFilePath = "compiler/example.gr";

    std::ifstream compilerFile(compilerFilePath);

    std::string line;
    std::string outputContent = "";

    while (std::getline(compilerFile, line)){
        ltrim(line); // Remove indents

        if (line.empty()){
            continue; // DO NOT PARSE EMPTY LINES
        }

        // Check the first and last characters
        // If everything is ok then pop front and back char
        // Now we have a string with just the command

        bool charOneCheck = false;
        bool charTwoCheck = false;

        if (line[0] == '(') {
            charOneCheck = true;
        }

        if (line.back() == ')') {
            charTwoCheck = true;
        }

        if (!charOneCheck || !charTwoCheck){
            std::cout << "Error context: " << line << std::endl;
            throw std::runtime_error("Syntax error, expected opening and closing parentheses");
        }

        line.pop_back();
        line.erase(0, 1);

        // First, check if the item referenced in the command is a builtin function
        // If not, look the function up in the current scope functions tree, then global function tree, if nothing is found
        // throw a runtime error

        std::vector<std::string> commandLineParsed = split(line, " ");

        if (commandLineParsed.at(0) == "import"){
            std::string importFileName = commandLineParsed.at(1);
            
            bool importIsStd = false;

            if (importFileName[0] == ':' && importFileName[1] == ':'){
                importIsStd = true;
                importFileName.erase(0, 2);
            }

            std::cout << importFileName << std::endl;

            if (importIsStd){
                // Read from the std/[FILENAME].gr

                std::ifstream file = std::ifstream("compiler/std/" + importFileName + ".gr");

                std::string fileContent = slurp(file);

                outputContent += fileContent;
            }
        }
    }

    std::cout << outputContent << std::endl;
}