#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <assert.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "error.h"
#include "source.h"
#include "dest.h"

class State;

// Contains all classes used to represent instructions in the MSP430 instruction set,
// plus a few 'virtual' instructions.

// Various References:
// http://www.ece.utep.edu/courses/web3376/Links_files/MSP430%20Quick%20Reference.pdf
// http://www.physics.mcmaster.ca/phys3b06/MSP430/MSP430_Instruction_Set_Summary.pdf
// http://homepages.ius.edu/RWISMAN/C335/HTML/MSP430/Chapt4.pdf
// *** http://www.physics.mcmaster.ca/phys3b06/MSP430/Instruction_Set.pdf
// http://www.ti.com.cn/cn/lit/ug/slau049f/slau049f.pdf


// Various Enumerations used in order to translate from assembly to an instruction
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
  // Virtual base class representing a single instruction.
public:

  virtual std::string toString() = 0;
  // Returns a string representation of this instruction

  virtual std::string byteStr() {
    // Returns the bytes corresponding to this instruction
    std::stringstream ss;
    for(int i = 0; i < size()/2; i++) {
      if(i != 0) ss << " ";
      ss << std::hex << std::setw(4) << std::setfill('0') << bytes[i];
    }
    return ss.str();
  }

  virtual std::string instructionName() = 0;
  // Returns the name of the instruction ('RRC','ADD', etc)

  virtual unsigned char size() { return 2; }
  // Return how many bytes this instruction took.

  virtual void execute(State* s);
  // Executes this instruction on the given MSP430 state.

  virtual ~Instruction() {}

  unsigned short bytes[3];
  // actual bytes representing this instruction
};

class VirtualInstruction : public Instruction {
  // Virtual instructions are just collections of instructions that
  // do not correspond to an actual instruction.  For example:
  // xxxx   ADD r15, 0xffff
  // xxxx+2 JNE xxxx
  // Will simplify to
  // CLEAR r15
  // These are just used in order to simplify the presentation in order to make
  // traces more understandable (so, you don't have 1000 lines of ADD,JNE, but just one CLEAR).
};

class MemcpyModify : public VirtualInstruction {
  // Virtual instruction representing a 'memcpy' that modifies it's arguments.
  // Equivalent to:
  // memcpy(r[dest], r[source], r[amount])
  // r[source] += amount
  // r[dest] += amount
  // r[amount] = 0
public:
  unsigned short source, dest, amount;
  unsigned short size_;

  virtual std::string toString();
  virtual unsigned char size() { return size_; }

  virtual std::string instructionName() { return "memcpy_modify"; }

  virtual void execute(State* s);

  MemcpyModify(unsigned short rx_, unsigned short ry_, unsigned short rz_, unsigned short size__):
    dest(rx_),source(ry_),amount(rz_),size_(size__) {
  }
};

class MemclearModify : public VirtualInstruction {
  // Virtual instruction representing a 'memcpy' that modifies it's arguments.
  // Equivalent to:
  // memclear(r[dest], r[amount])
  // r[dest] += amount
  // r[amount] = 0

public:
  unsigned short dest, amount;
  unsigned short size_;

  virtual std::string toString();
  virtual unsigned char size() { return size_; }

  virtual std::string instructionName() { return "memclear_modify"; }

  virtual void execute(State* s);

  MemclearModify(unsigned short rx_, unsigned short rz_, unsigned short size__):
    dest(rx_),amount(rz_),size_(size__) {
  }
};

class Clear : public VirtualInstruction {
  // Virtual instruction representing a 'clear'.
  // Equivalent to:
  // r[r] = 0

public:
  unsigned short r;
  unsigned short size_;

  virtual std::string toString();
  virtual unsigned char size() { return size_; }
  virtual std::string instructionName() { return "clear"; }

  virtual void execute(State* s);

  Clear(unsigned short r_, unsigned short size__):
    r(r_),size_(size__) {
  }
};

class InstructionList : public VirtualInstruction {
  // Generic VirtualInstruction; has a list of instructions to execute when it is executed
  // but a different string representation.  Used for more complicated virtual instructions.
public:
  std::vector<Instruction*> instructions;
  std::string name;
  std::string toString_;

  virtual unsigned char size() { return 0; }
  virtual std::string toString() { return toString_; }
  virtual std::string instructionName() { return name; }
  virtual void execute(State* s);

  InstructionList(std::string name):
    name(name) {}
};

// From here on down are actual instructions that the MSP430 can perform.
class Condition : public Instruction {
 public:
  virtual std::string toString();

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
  virtual std::string toString();
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
  virtual std::string toString();

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
