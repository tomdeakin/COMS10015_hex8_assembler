// Copyright (c) 2020 Tom Deakin
// SPDX-License-Identifier: MIT

#include <map>
#include <iostream>
#include <string>
#include <list>

// Defines the type of operand, particular for labels
// If immediate, the label line number will be used
// If offset, the relative difference between the current line and label will be used - all instructions which update pc have this property
enum class operandType { none, immediate, offset };

// Structure of an instruction
// Opcode plus an operand of a particular type
class instruction {
  public:
  std::string inst;
  int opcode;
  operandType type;

  instruction(std::string _inst, int _opcode, operandType _type) : inst(_inst), opcode(_opcode), type(_type) {};
};

// Structure of an output line
// If a label needs resolving, we'll store that label here.
class line {
  public:
  std::string opcode;
  int operand;
  bool requiresLabelResolution = false;
  std::string label;
  int data;
  bool isData = false;
};

// The definition of the Hex 8 ISA 
class ISA {

  // Instruction table for the Hex 8 ISA
  const std::map<std::string, instruction> table = {
    {"ldam", {"ldam", 0x0, operandType::immediate}},
    {"ldbm", {"ldbm", 0x1, operandType::immediate}},
    {"stam", {"stam", 0x2, operandType::immediate}},
    {"ldac", {"ldac", 0x3, operandType::immediate}},
    {"ldbc", {"ldbc", 0x4, operandType::immediate}},
    {"ldap", {"ldap", 0x5, operandType::offset}},
    {"ldai", {"ldai", 0x6, operandType::immediate}},
    {"ldbi", {"ldbi", 0x7, operandType::immediate}},
    {"stai", {"stai", 0x8, operandType::immediate}},
    {"br",   {"br",   0x9, operandType::offset}},
    {"brz",  {"brz",  0xA, operandType::offset}},
    {"brn",  {"brn",  0xB, operandType::offset}},
    {"brb",  {"brb",  0xC, operandType::none}},
    {"add",  {"add",  0xD, operandType::none}},
    {"sub",  {"sub",  0xE, operandType::none}},
    {"pfix", {"pfix", 0xF, operandType::immediate}}
  };

  public:

  // Look up an opcode in the table
  const instruction& getInstruction(const std::string& inst) const {
    try {
      return table.at(inst);
    } catch (std::out_of_range& err) {
      std::cerr << "Error: Invalid instruction in program - " << inst << std::endl;
      exit(EXIT_FAILURE);
    }
  };

  // Check if instruction is in the Hex 8 ISA
  bool validInstruction(const std::string& inst) {
    return table.count(inst);
  }

  // Check if instruction is a DATA entry
  bool validData(const std::string& inst) {
    return !inst.compare("data");
  }

  // Check if instruction is a label
  // Labels are of the form L12345 (unspecified length)
  bool validLabel(const std::string& inst) {
    if (inst[0] != 'l') return false;

    auto rest = inst.substr(1);
    return std::all_of(rest.begin(), rest.end(), ::isdigit);
  }
};


// Hold the mapping of labels to output line numbers
class assemblyLabels {
  // Table to hold the labels and their target output line numbers
  std::map<std::string, int> labels;

  public:

  // Add the evaluated labels to the label table
  void setLabels(const std::vector<std::string>& labelsToStore, const int outLineNum) {
    for (auto l : labelsToStore) {
      // Check if label already exists and throw error
      if (labels.count(l)) {
        std::cerr << "Error: Label redefined - " << l << std::endl;
        exit(EXIT_FAILURE);
      } else {
          labels[l] = outLineNum;
      }
    }
  }

  // Check if label is in table
  bool seenLabel(const std::string& label) {
    return labels.count(label);
  }

  // Display the number of labels in the table
  void printLabelCount() {
    std::cout << "Number of labels: " << labels.size() << std::endl;
  }

  // Display the label table
  void printLabels() {
    for (auto l : labels) {
      std::cout << l.first << " -> line " << l.second << std::endl;
    }
  }

  // Get the label value, depending on the instruction type
  int resolveLabel(const class ISA& hex8, const std::string& label, const int outLineNum, const std::string& instName) {
    // Get label number from label table
    if (!labels.count(label)) {
        std::cerr << "Error: unknown label - " << label << std::endl;
        exit(EXIT_FAILURE);
    }
    int labelValue = labels[label];

    // Look up instruction
    auto inst = hex8.getInstruction(instName);

    switch (inst.type) {
      case operandType::immediate:
        return labelValue;
      case operandType::offset:
        return labelValue - outLineNum - 1;
      case operandType::none:
        std::cerr << "Error: instruction should not have a label - " << label << std::endl;
        exit(EXIT_FAILURE);
    };
  }

};

class hexOutputStream {
  // Stream of instructions to output
  public:
  std::list<line> outputStream;

  // Add instruction to the output
  void emitInstruction(const line& outputLine) {
    outputStream.push_back(outputLine);
  }

  // Get length of output stream
  int getOutputLength() {
    return outputStream.size();
  }
};
