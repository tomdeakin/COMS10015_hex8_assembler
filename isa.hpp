#include <map>
#include <iostream>
#include <string>

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
  
  instruction() {};

  instruction(std::string _inst, int _opcode, operandType _type) : inst(_inst), opcode(_opcode), type(_type) {};
};

// The definition of the Hex 8 ISA 
class ISA {
  // Table to hold the instructions in Hex 8
  std::map<std::string, instruction> table;

  // Table to hold the labels and their target output line numbers
  std::map<std::string, int> labels;

  // Create the instruction table for the Hex 8 ISA
  public:
  ISA() {
    table["ldam"] = instruction("ldam", 0x0, operandType::immediate);
    table["ldbm"] = instruction("ldbm", 0x1, operandType::immediate);
    table["stam"] = instruction("stam", 0x2, operandType::immediate);
    table["ldac"] = instruction("ldac", 0x3, operandType::immediate);
    table["ldbc"] = instruction("ldbc", 0x4, operandType::immediate);
    table["ldap"] = instruction("ldap", 0x5, operandType::offset);
    table["ldai"] = instruction("ldai", 0x6, operandType::immediate);
    table["ldbi"] = instruction("ldbi", 0x7, operandType::immediate);
    table["stai"] = instruction("stai", 0x8, operandType::immediate);
    table["br"]   = instruction("br",   0x9, operandType::offset);
    table["brz"]  = instruction("brz",  0xA, operandType::offset);
    table["brn"]  = instruction("brn",  0xB, operandType::offset);
    table["brb"]  = instruction("brb",  0xC, operandType::none);
    table["add"]  = instruction("add",  0xD, operandType::none);
    table["sub"]  = instruction("sub",  0xE, operandType::none);
    table["pfix"] = instruction("pfix", 0xF, operandType::immediate);
  };

  // Look up an opcode in the table
  const instruction& getInstruction(const std::string& inst) {
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

  // Get the label value, depending on the instruction type
  int resolveLabel(const std::string& label, const int outLineNum, const instruction& inst) {
    // Get label number from label table
    if (!labels.count(label)) {
        std::cerr << "Error: unknown label - " << label << std::endl;
        exit(EXIT_FAILURE);
    }
    int labelValue = labels[label];

    std::cout << label << " = " << labelValue << " on line " << outLineNum << std::endl;

    switch (inst.type) {
      case operandType::immediate:
        return labelValue;
      case operandType::offset:
        return labelValue - outLineNum;
      case operandType::none:
        std::cerr << "Error: instruction should not have a label - " << label << std::endl;
        exit(EXIT_FAILURE);
    };
  }

  void printLabelCount() {
    std::cout << "Number of labels: " << labels.size() << std::endl;
  }
};
