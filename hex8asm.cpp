#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>


#include "isa.hpp"

// Output file name
static constexpr char outputFile[] = "a.x";


// Main routine
int main(int argc, char *argv[]) {

  // Check number of arguments
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " prog.hex8" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Load input file from command line argument
  std::ifstream progSource(argv[1]);
  if (!progSource.is_open()) {
    std::cerr << "Could not open file: " << argv[1] << std::endl;
    exit(EXIT_FAILURE);
  }

  // Load output file, using fixed name
  std::ofstream hexOutput(outputFile);
  if (!hexOutput.is_open()) {
    std::cerr << "Could not open output file " << outputFile << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set up Hex 8 ISA
  ISA hex8;

  // First pass
  // Loop over the file and collect and evaluate labels
  // If there are any invalid instructions, we stop processing
  {
    std::string line;
    int srcLineNum = 0;
    int outLineNum = 0;
    std::vector<std::string> outstandingLabels;
    while (std::getline(progSource, line)) {

      // Skip blank lines
      if (!line.size()) {
        srcLineNum++;
        continue;
      }

      // Loop over line and construct a list (vector) of words on the line
      std::stringstream ss(line);
      std::vector<std::string> tokens;
      for (std::string t; ss >> t;) {
        // Convert to lower case
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);

        // Add to list of tokens
        tokens.push_back(t);
      }

      // Check only for labels
      if (hex8.validLabel(tokens[0])) {
        // Add to label list
        outstandingLabels.push_back(tokens[0]);
      } else if (hex8.validData(tokens[0]) || hex8.validInstruction(tokens[0])) {
        // Check if the line is data or an instruction
        // If so, then we now have the next output line number to resolve the label

        // Resolve the labels
        hex8.setLabels(outstandingLabels, outLineNum);

        // Empty the list of labels to process
        outstandingLabels.clear();

        outLineNum++;

        // Check if operand is negative, and if so we need to insert a prefix instruction
        if (hex8.validInstruction(tokens[0])) {
          auto inst = hex8.getInstruction(tokens[0]);
          if (inst.type != operandType::none) {
            if (tokens.size() != 2) {
              std::cerr << "Error: instruction need an operand (line " << srcLineNum+1 << ") - " << line << std::endl;
              exit(EXIT_FAILURE);
            }
            if (!hex8.validLabel(tokens[1])) {
              int opcode = std::stoi(tokens[1]);
              if (opcode < 0) {
                outLineNum++;
              }
            } else {
              // If already seen label and instruction takes an offset, then resolved label may be negative
              // If it is, need to add a prefix instruction
              if (inst.type == operandType::offset) {
                if (hex8.seenLabel(tokens[1])) {
                  outLineNum++;
                }
              }
              // TODO, might be a long jump of an unseen label, in which case we also have a prefix
            }
          }
        }

      } else {
        // Otherwise, the line contains something unexpected
        std::cerr << "Error: Unknown instruction (line " << srcLineNum+1 << ") - " << line << std::endl;
        exit(EXIT_FAILURE);
      }

      srcLineNum++;
    }

    // Print out information about the first pass
    std::cout << "Pass 1 successful" << std::endl;
    std::cout << "Lines of source: " << srcLineNum << std::endl;
    hex8.printLabelCount();
  }

  {
    // Second pass
    // Reset the input file
    progSource.clear();
    progSource.seekg(progSource.beg);
    
    int outLineNum = 1;
    int srcLineNum = 0;
    std::string line;

    // Loop over lines
    while (std::getline(progSource, line)) {
      srcLineNum++;
      
      // Skip blank lines
      if (!line.size()) {
        continue;
      }

      // Loop over line and construct a list (vector) of words on the line
      std::stringstream ss(line);
      std::vector<std::string> tokens;
      for (std::string t; ss >> t;) {
        // Convert to lower case
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);

        // Add to list of tokens
        tokens.push_back(t);
      }

      // Skip labels
      if (hex8.validLabel(tokens[0])) {
        continue;
      } else if (hex8.validData(tokens[0])) {

        // Found a line of the form DATA 128
        // Output the operand in Hex
        if (tokens.size() != 2) {
          std::cerr << "Error: Illformed DATA (line " << srcLineNum+1 << ") - " << line << std::endl;
          exit(EXIT_FAILURE);
        }

        // Convert operand to integer
        int data = std::stoi(tokens[1]);

        // Check data is in valid range
        // There are 2^8 bits, which is either unsigned or signed (2s complement)
        if (data > 255 || data < -128) {
          std::cerr << "Error: DATA out of range (line " << srcLineNum+1 << ") - " << line << std::endl;
          exit(EXIT_FAILURE);
        }

        // Output the value in hex
        if (data == 0) {
          hexOutput << "00 ";
        } else {
          hexOutput << std::hex << std::uppercase << (data & 0xFF) << " ";
        }
        outLineNum++;

      } else if (hex8.validInstruction(tokens[0])) {
        // Found an instruction
        // Look up the information about it from ISA table
        auto inst = hex8.getInstruction(tokens[0]);

        if (inst.type != operandType::none && tokens.size() != 2) {
          std::cout << "Error: Illformed instruction (line " << srcLineNum+1 << ") - " << line << std::endl;
          exit(EXIT_FAILURE);
        }

        // Check if opcode is a label
        if (hex8.validLabel(tokens[1])) {
          // Evaluate label
          std::cout << line << std::endl;
          int label = hex8.resolveLabel(tokens[1], outLineNum, inst);
          if (label < 0) {
            // Output prefix instruction
            // TODO
          }
          hexOutput << std::hex << std::uppercase << (inst.opcode & 0xF) << (label & 0xF) << " ";
          outLineNum++;
        } else {
          if (inst.type == operandType::none) {
            hexOutput << std::hex << std::uppercase << (inst.opcode & 0xF) << "0 ";
            outLineNum++;
          } else {
            int opcode = std::stoi(tokens[1]);
            if (opcode < 0) {
              // Output Prefix instruction first
              hexOutput << std::hex << std::uppercase << (hex8.getInstruction("pfix").opcode & 0xF)
                << (((opcode & 0xFF) >> 0x4) & 0xF)
                << " ";
              outLineNum++;
            }
            hexOutput << std::hex << std::uppercase << (inst.opcode & 0xF) << (opcode & 0xF) << " ";
            outLineNum++;
          }
        }
      }
    }
  }

  // Loader
  // This sets up the binary format to include the stack and instructions
  // The first entry is a PFIX instruction
  // TODO
  // The value will depend on the first jump instruction (if any)

  return EXIT_SUCCESS;
}
