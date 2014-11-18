#ifndef STATE_H
#define STATE_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include "instruction.h"

class State {
public:

  std::string lastFile;
  std::string lastDataFile;

  class Data {
  public:
    unsigned char memory[0xffff];
    unsigned short r[16];

    bool locked;
    bool running;
    bool dep_on;

    bool page_writable[256];
  };

  Data data;
  Data prev_data[100];
  unsigned int data_idx ;

  bool watchpoint_triggered;
  bool exit_on_finished;

  std::vector<std::string> input;
  int input_idx;

  std::map<unsigned short, std::string> label;
  std::map<unsigned short, std::string> section;
  std::map<unsigned short, bool> breakpoint;
  std::map<unsigned short, unsigned char> watchpoint;
  std::map<unsigned char, unsigned short> watchpointRegister;

  unsigned short strToReg(std::string str);
  void writeByte(unsigned short address, unsigned char value);

  Instruction* instructionForAddr(unsigned short addr);
  Source* sourceOperand(unsigned short as, unsigned short source, unsigned short addr);
  Dest* destOperand(unsigned short ad, unsigned short dest, unsigned short addr);
  unsigned short readWord(unsigned short addr);
  unsigned char readByte(unsigned short addr);
  void readFile(std::string filename);
  void readMemoryDump(std::string filename);
  void list();
  void step();
  void reverse();
  void reset(bool rereadFile=true);

  State() {
    input_idx = 0;
    data_idx = 0;
    exit_on_finished = false;
    reset();
  }
};

extern State* s;


#endif
