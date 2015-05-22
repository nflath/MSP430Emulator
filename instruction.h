
// http://www.ece.utep.edu/courses/web3376/Links_files/MSP430%20Quick%20Reference.pdf
// http://www.physics.mcmaster.ca/phys3b06/MSP430/MSP430_Instruction_Set_Summary.pdf
// http://homepages.ius.edu/RWISMAN/C335/HTML/MSP430/Chapt4.pdf
// *** http://www.physics.mcmaster.ca/phys3b06/MSP430/Instruction_Set.pdf
// http://www.ti.com.cn/cn/lit/ug/slau049f/slau049f.pdf

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <assert.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "error.h"
#include "source.h"
#include "dest.h"

class State;

enum ConditionCode {
  JNE_,
  JEQ_,
  JNC_,
  JC_,
  JN_,
  JGE_,
  JL_,
  JMP_
};

enum OpCode_TwoOperand {
  MOV_  = 0b0100,
  ADD_  = 0b0101,
  ADDC_ = 0b0110,
  SUBC_ = 0b0111,
  SUB_  = 0b1000,
  CMP_  = 0b1001,
  DADD_ = 0b1010,
  BIT_  = 0b1011,
  BIC_  = 0b1100,
  BIS_  = 0b1101,
  XOR_  = 0b1110,
  AND_  = 0b1111
};

enum OpCode_OneOperand {
  RRC_ = 0b000,
  SWPB_ = 0b001,
  RRA_ = 0b010,
  SXT_ = 0b011,
  PUSH_ = 0b100,
  CALL_ = 0b101,
  RETI_ = 0b110
};

class Instruction {
 public:
  virtual std::string toString() = 0;
  virtual std::string instructionName() = 0;
  virtual unsigned char size() { return 2; }
  virtual void execute(State* s);
  virtual ~Instruction() {}
  std::string sourceString;
};

class Condition : public Instruction {
 public:
  virtual std::string toString() {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << (addr+2+offset*2);
    return instructionName() + "\t" + ss.str();
  }

  unsigned short addr;
  signed short offset;

};

class JNE : public Condition {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JNE";}
};

class JEQ : public Condition {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JEQ";}
};

class JNC : public Condition {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JNC";}
};

class JC : public Condition {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JC";}
};

class JN : public Condition {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JN";}
};

class JGE : public Condition {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JGE";}
};

class JL : public Condition {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JL";}
};

class JMP : public Condition {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() { return "JMP";}
};

class InstructionOneOperand : public Instruction {
 public:
  virtual std::string toString() {
    // FixMe: Implement
    return instructionName() + (byte ? ".b\t" : "\t") + source->toString();
  }
  bool byte;

  virtual unsigned char size() { return 2 + source->size(); }

  Source* source;
};

class RRC : public InstructionOneOperand {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "RRC";
  }
};

class SWPB : public InstructionOneOperand {
  virtual void execute(State* s);
 public:
  virtual std::string instructionName() {
    return "SWPB";
  }
};

class RRA : public InstructionOneOperand {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "RRA";
  }
};

class SXT : public InstructionOneOperand {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "SXT";
  }
};

class PUSH : public InstructionOneOperand {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "PUSH";
  }
};

class CALL : public InstructionOneOperand {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "CALL";
  }
};

class RETI : public InstructionOneOperand {
 public:
  virtual std::string instructionName() {
    return "RETI";
  }
};

class InstructionTwoOperands : public Instruction {
 public:
  virtual std::string toString() {
    std::string str = instructionName() + (byte ? ".b\t" : "\t") + source->toString() + ", " + dest->toString();
    if(str == "MOV\t@sp+, PC") {
      return "RET";
    }
    return str;
  }

  virtual unsigned char size() { return 2 + source->size() + dest->size(); }

  bool byte; // .B version or not
  Source* source;
  Dest* dest;

  unsigned char numBytes;
};

class MOV : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "MOV";
  }
};

class ADD : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "ADD";
  }
};



class ADDC : public InstructionTwoOperands {
 public:
  virtual std::string instructionName() {
    return "ADDC";
  }
};

class SUBC : public InstructionTwoOperands {
 public:
  virtual std::string instructionName() {
    return "SUBC";
  }
};

class SUB : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "SUB";
  }
};

class CMP : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);

  virtual std::string instructionName() {
    return "CMP";
  }
};

class DADD : public InstructionTwoOperands {
 public:
  virtual std::string instructionName() {
    return "DADD";
  }

  virtual void execute(State* s);
};

class BIT : public InstructionTwoOperands {
public:
  virtual void execute(State* s);
  virtual std::string instructionName() {
    return "BIT";
  }
};

class BIC : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);

  virtual std::string instructionName() {
    return "BIC";
  }
};

class BIS : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);

  virtual std::string instructionName() {
    return "BIS";
  }
};

class XOR : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);

  virtual std::string instructionName() {
    return "XOR";
  }
};

class AND : public InstructionTwoOperands {
 public:
  virtual void execute(State* s);

  virtual std::string instructionName() {
    return "AND";
  }
};

class INTERRUPT : public Instruction {
  virtual void execute(State* s);
  virtual std::string toString();

  virtual std::string instructionName() {
    return "ret";

  }
};

#endif
