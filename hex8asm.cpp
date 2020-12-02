// Copyright (c) 2020 Tom Deakin
// SPDX-License-Identifier: MIT

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cassert>


#include "hex8asm.hpp"

// Output file name
static constexpr char outputFile[] = "a.hex";


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

  // Create table of labels
  assemblyLabels labels;

  // Create hex output stream
  hexOutputStream outputStream;

  // First pass
  // Loop over the file and translate each instruction
  // When labels are defined, save them along with the output line number
  // Long backwards jumps stay correctly referenced because we (non-optimally)
  // always output a PFIX instruction before each intruction which uses a label.
  // Any instructions which forward reference a label need to be processed on a second pass.
  // If there are any invalid instructions, we stop processing
  {
    std::string line;
    int srcLineNum = 0;
    int outLineNum = 0;
    std::vector<std::string> labelDeclarations;
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
        labelDeclarations.push_back(tokens[0]);
      } else if (hex8.validData(tokens[0])) {
        // Process the data instructions
        // Found a line of the form DATA xxx

        // Now found a non-label, so know target of labels is this current output line
        labels.setLabels(labelDeclarations, outLineNum);
        // Empty the list of labels to process
        labelDeclarations.clear();
        assert(outLineNum == outputStream.getOutputLength());

        // Check that it has an operand.
        if (tokens.size() != 2) {
          std::cerr << "Error: Illformed DATA (line " << srcLineNum+1 << ") - " << line << std::endl;
          exit(EXIT_FAILURE);
        }

        // Convert operand to integer
        int data = std::stoi(tokens[1]);

        // Data instructions are just 8-bit values, so we can just output them
        class line output;
        output.data = (data & 0xFF);
        output.isData = true;
        outputStream.emitInstruction(output);
        outLineNum++;



      } else if (hex8.validInstruction(tokens[0])) {
        // Process a regular instruction
        auto inst = hex8.getInstruction(tokens[0]);

        // Now found a non-label, so know target of labels is the next output line
        labels.setLabels(labelDeclarations, outLineNum);
        // Empty the list of labels to process
        labelDeclarations.clear();

        if (inst.type != operandType::none) {
          // Instruction needs an operand, so check it has one
          if (tokens.size() != 2) {
            std::cerr << "Error - instruction missing operand (line " << srcLineNum+1 << ") - " << line << std::endl;
            exit(EXIT_FAILURE);
          }

          // If operand is a label, we emit a prefix instruction just in case we need all 8-bits
          // Note this is not that optimal, but an iterative pass could remove redundant ones
          if (hex8.validLabel(tokens[1])) {
            class line prefix;
            prefix.opcode = "pfix";
            prefix.requiresLabelResolution = true;
            prefix.label = tokens[1];
            outputStream.emitInstruction(prefix);
            outLineNum++;

            // Then emit this instruction
            class line output;
            output.opcode = tokens[0];
            output.requiresLabelResolution = true;
            output.label = tokens[1];
            outputStream.emitInstruction(output);
            outLineNum++;
            assert(outLineNum == outputStream.getOutputLength());
          } else {
            // Otherwise, operand is just an integer

            // Check if we need a prefix, which will be is operand is negative or large
            // Convert operand to integer
            int operand = std::stoi(tokens[1]);

            if (operand < 0 || operand > 15) {
              // Emit prefix
              class line prefix;
              prefix.opcode = "pfix";
              prefix.operand = ((operand & 0xFF) >> 0x4); // 4 high bits
              outputStream.emitInstruction(prefix);
              outLineNum++;
            }

            // Output the instruction
            class line output;
            output.opcode = tokens[0];
            output.operand = (operand & 0xF); // 4 low bits
            outputStream.emitInstruction(output);
            outLineNum++;
            assert(outLineNum == outputStream.getOutputLength());
          }
        } else {
          // Instruction has no operand
          class line output;
          output.opcode = tokens[0];
          output.operand = 0;
          outputStream.emitInstruction(output);
          outLineNum++;
          assert(outLineNum == outputStream.getOutputLength());
        }
      }
      srcLineNum++;
    } // End of line loop

    // Print out information about the first pass
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "Pass 1 successful" << std::endl;
    std::cout << "Lines of source: " << srcLineNum << std::endl;
    std::cout << std::endl;
    labels.printLabelCount();
    labels.printLabels();
    std::cout << "--------------------------------------------------------------------------------" << std::endl << std::endl;
  } // End of pass 1


  {
    // Second pass
    // Now we just loop over the output instructions, and resolve any outstanding labels
    int outLineNum = 0;
    for (auto out = outputStream.outputStream.begin(); out != outputStream.outputStream.end(); ++out) {
      auto output = *out;
      if (output.requiresLabelResolution) {
        // Will be either a prefix or an instruction
        if (output.opcode == "pfix") {
          // Set the operand to be the 4 high bits of the label
          // Label offsets will be from the next line, so +1 to the line number
          output.operand = ((labels.resolveLabel(hex8, output.label, outLineNum+1, std::next(out)->opcode) >> 0x4) & 0xF);
        } else {
          // Set the operand to be the 4 low bits of the label
          output.operand = (labels.resolveLabel(hex8, output.label, outLineNum, output.opcode) & 0xF);
        }
      }

      if (output.isData) {
        if (output.data == 0) {
          hexOutput << "00 ";
          std::cout << std::dec << outLineNum << ": DATA 0" << std::endl;
        } else if (output.data <= 0xF) {
          hexOutput << std::hex << std::uppercase << "0" << (output.data & 0xF) << " ";
          std::cout << std::dec << outLineNum << ": DATA " << std::hex << std::uppercase << (output.data & 0xFF) << std::endl;
        } else {
          hexOutput << std::hex << std::uppercase << (output.data & 0xFF) << " ";
          std::cout << std::dec << outLineNum << ": DATA " << std::hex << std::uppercase << (output.data & 0xFF) << std::endl;
        }
      } else {
        hexOutput << std::hex << std::uppercase << (hex8.getInstruction(output.opcode).opcode & 0xF) << (output.operand & 0xF) << " ";
        std::cout << std::dec << outLineNum << ": " << output.opcode << " " << std::hex << std::uppercase << (output.operand & 0xF) << std::endl;
      }
      outLineNum++;
    }

    // Print out information about the first pass
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "Pass 2 successful" << std::endl;
    std::cout << "Number of instructions output: " << std::dec << outLineNum << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl << std::endl;
  } // End of pass 2

  return EXIT_SUCCESS;
}
