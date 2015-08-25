#ifndef STATE_H
#define STATE_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include "instruction.h"

class State {
  // Class representing the actual state of the MSP430 CPU and debugger.
public:

  class Data {
    // The actual data of the CPU itself.
  public:
    unsigned char memory[0xffff];
    // Main memory

    unsigned short r[16];
    // Registers

    bool locked;
    // Whether the CPU is locked or unlocked

    bool running;
    // Whether the CPU is still running

    bool dep_on;
    // Whether DEP is enabled on this CPU

    bool page_writable[256];
    // Whether each pacge is writable or executable

    bool use_rand;
    // Whether to generate random numbers as 'rand' or actually random
    unsigned short rand;
    // random number to generate

    bool printed;
    bool inputed;
    // Flags used to determine whether to return to the debug prompt (for
    // --break-on-print and --break-on-input)
  };

  Data data;
  // Current state of the CPU

  Data prev_data[100];
  // Last 100 states of the CPU; used for reversible debugging.
  unsigned int data_idx;
  // index into prev_data of the next element to write

  std::string lastDataFile;
  // Last binary file we read; used for 'reset'

  unsigned short read[500];
  // List of memory locations read this step.  Used for read breakpoints.
  unsigned short read_idx;
  // Index into the 'read' array.  The next element to write.

  bool watchpoint_triggered;
  // Whether a watchpoint was triggered in the last step.
  bool exit_on_finished;
  // Whether or not this emulator should exit when the CPU stops running or not.

  std::vector<std::string> input;
  // Queue of 'next input' strings to use
  int input_idx;
  // index into the input string list

  std::map<unsigned short, bool> breakpoint;
  std::map<unsigned short, bool> read_breakpoint;
  std::map<unsigned short, unsigned char> watchpoint;
  std::map<unsigned char, unsigned short> watchpointRegister;
  // List of various types of breakpoints:
  //   breakpoint - break when the PC is at that address
  //   read_breakpoint - break when a memory address is read
  //   watchpoint - break when a memory address is modified
  //   watchpointRegister - break when a register is modified

  void writeByte(unsigned short address, unsigned char value);
  // Write a byte to the given address - checks writable flag.

  void compare(std::string filename);
  // Compares the state of a CPU to the state of a CPU described by the given binary file.

  Instruction* instructionForAddr(unsigned short addr);
  // Return the instruction at the given address.  Can return virtual instructions, if one
  // matches.

  Instruction* realInstructionForAddr(unsigned short addr);
  // Returns the instruction at the given address.  Does not return virtual instructions

  Source* sourceOperand(unsigned short as, unsigned short source, unsigned short addr);
  // Returns a source for the given addressing modes/address

  Dest* destOperand(unsigned short ad, unsigned short dest, unsigned short addr);
  // Returns a dest for the given addressing modes/address

  unsigned short readWord(unsigned short addr, bool byte_=false);
  // Read and return a word in memory

  unsigned char readByte(unsigned short addr);
  // Read and return a byte in memory

  void readMemoryDump(std::string filename);
  // Read a binary file and set the state based on it.

  void createMemoryDump();
  // Create a binary file based on the current state.

  void step();
  // Execute one instruction
  void reverse();

  // Reverse-step one instruction
  void reset(bool rereadFile=true);
  // Reset to the last good state.

  State() {
    input_idx = 0;
    data_idx = 0;
    exit_on_finished = false;
    reset();
  }
};

extern State* s;


#endif
