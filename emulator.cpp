// http://en.wikipedia.org/wiki/TI_MSP430#MSP430_CPU
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <assert.h>
#include "emulator.h"
#include "source.h"
#include "dest.h"
#include "state.h"
#include "instruction.h"
#include "util.h"

extern State* s;

std::ostream& operator<<(std::ostream& os, const OpCode_TwoOperand& obj) {
  switch(obj) {
  case MOV_: std::cout << "MOV"; break;
  case ADD_: std::cout << "ADD"; break;
  case ADDC_:std::cout << "ADDC"; break;
  case SUBC_:std::cout << "SUBC"; break;
  case SUB_:std::cout << "SUB"; break;
  case CMP_:std::cout << "CMP"; break;
  case DADD_:std::cout << "DADD"; break;
  case BIT_:std::cout << "BIT"; break;
  case BIC_:std::cout << "BIC"; break;
  case BIS_:std::cout << "BIS"; break;
  case XOR_:std::cout << "XOR"; break;
  case AND_:std::cout << "AND"; break;
  }
  return os;
}

int
strToR(const std::string str) {
  if(str == "pc") {
    return 0;
  } else if(str == "sr") {
    return 2;
  } else if(str == "sp") {
    return 1;
  }
  if(str[0] == 'r') {
    unsigned long value = strtoul(str.substr(1).c_str(),NULL,10);
    return value;
  }

  return -1;
}


void
execShellCommand(State* s, std::string command, std::string args) {
  if(command == "echo") {
    std::cout << args << std::endl;
  } else if(command == "load") {
    s->readMemoryDump(args);
  } else if(command == "dump") {
    s->createMemoryDump();
  } else if(command == "list") {
    s->list();
  } else if(command == "reset") {
    s->reset();
  } else if(command == "compare") {
    s->compare(args);
  } else if(command == "break" || command == "br") {
    std::stringstream ss(args);
    unsigned short addr;
    ss >> std::hex >> addr;
    s->breakpoint[addr] = true;
  } else if(command == "remove" || command == "rb") {
    if(args[0] == 'r') {
      s->watchpointRegister.erase(strToR(args.substr(1)));
    } else {
      std::stringstream ss(args);
      unsigned short addr;
      ss >> std::hex >> addr;
      s->breakpoint.erase(addr);
      s->watchpoint.erase(addr);
    }
  } else if(command == "watch") {
    if(args[0] == 'r') {

      s->watchpointRegister[strToR(args.substr(1))] = s->data.r[strToR(args.substr(1))];

    } else {
      std::stringstream ss(args);
      unsigned short addr;
      ss >> std::hex >> addr;
      s->watchpoint[addr] = s->data.memory[addr];
    }
  } else if(command == "c" || command == "continue") {
    int numSteps = 1;

    for(int i = 0; i < numSteps; i++) {
      while(s->data.running &&
            s->data.locked &&
            !s->watchpoint_triggered) {
        s->step();
        if(s->breakpoint[s->data.r[0]]) {
          break;
        }
      }
    }

    if(s->data.running) {
      Instruction* i = s->instructionForAddr(s->data.r[0]);
      std::cout << std::hex << s->data.r[0] << ": " << i->toString() << std::endl;
    }

  } else if(command == "f" || command == "finish") {
    while(s->data.running &&
          s->data.locked &&
          !s->watchpoint_triggered&&
          s->readWord(s->data.r[0]) != 0x4130) {
      s->step();
      if(s->breakpoint[s->data.r[0]]) {
        break;
      }
    }
    if(s->data.running &&
       (s->readWord(s->data.r[0]) == 0x4130)) {
      s->step();
    }
    if(s->data.running) {
      Instruction* i = s->instructionForAddr(s->data.r[0]);
      std::cout << std::hex << s->data.r[0] << ": " << i->toString() << std::endl;
    }
  } else if(command == "s" || command == "step") {
    if(args.length() > 0) {
      std::stringstream ss(args);
      int max;
      ss >> max;
      for(int i = 0; i < max; i++) {
        s->step();
      }
    } else {
      s->step();
    }
    std::cout << std::hex << s->data.r[0] << ": "<< std::flush; //
    Instruction* i = s->instructionForAddr(s->data.r[0]);
    std::cout << i->toString() << std::endl;
    delete i;
  } else if(command == "r" || command == "reverse") {
    if(args.length() > 0) {
      std::stringstream ss(args);
      int max;
      ss >> max;
      for(int i = 0; i < max; i++) {
        s->reverse();
      }
    } else {
      s->reverse();
    }
    std::cout << std::hex << s->data.r[0] << ": "<< std::flush;
    Instruction* i = s->instructionForAddr(s->data.r[0]);
    if(i) {
      std::cout << i->toString() << std::endl;
    } else {
      std::cout << " Invalid instruction" << std::endl;
    }
    delete i;
  } else if(command == "p") {
    if(args[0] == '*') {
      if(args == "*pc") {
        Instruction* i = s->instructionForAddr(s->data.r[0]);
        std::cout << std::hex << s->data.r[0] << ": " << i->toString() << std::endl;
        delete i;
      } else {
        int value = strToR(args.substr(1));
        if(value < 0) {
          unsigned short addr = strtoul(args.substr(1).c_str(), NULL, 16);
          std::cout << args << ": " << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)s->data.memory[addr] << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)s->data.memory[addr+1] << std::endl;
        } else {
          std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)s->data.memory[s->data.r[value]] << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)s->data.memory[s->data.r[value]+1] << std::endl;
        }
      }
    } else {
      int value = strToR(args);
      std::cout << std::hex << s->data.r[value] << std::endl;
    }
  } else if (command == "run") {
    std::ifstream f(args.c_str());
    if (f.is_open()) {
      std::string newCommand;
      std::string newArgs;
      while( f >> newCommand) {
        getline(f,newArgs);
        newArgs.erase(newArgs.find_last_not_of(" \n\r\t")+1);
        newArgs.erase(0, newArgs.find_first_not_of(" \n\r\t"));
        std::cout << ">> " << newCommand << " " << newArgs << std::endl;
        execShellCommand(s,newCommand,newArgs);
      }
    } else {
      std::cout << "Unable to open file: " << args << std::endl;
    }
  } else if(command == "input") {
    s->input.push_back(args);
  } else if(command == "exit-on-finished") {
    s->exit_on_finished = true;
  } else {
    std::cout << "Unknown command: '" << command << "'" << std::endl;
  }
}

void
shell(State* s) {
  std::cout << ">> " << std::flush;
  std::string input;

  while((input = readString()) != "") {
    std::string args = readLine();
    args.erase(args.find_last_not_of(" \n\r\t")+1);
    args.erase(0, args.find_first_not_of(" \n\r\t"));
    execShellCommand(s,input, args);
    if(!s->data.locked) {
      std::cout << std::endl << "Door unlocked!" << std::endl;
    } else if (!s->data.running) {
      std::cout << std::endl << "CPU no longer running." << std::endl;
    }
    std::cout << ">> " << std::flush;
  }
}

int
main(int argc, char** argv) {

  if(argc == 1) {
  } else if (!strcmp(argv[1],"-r")) {
    execShellCommand(s, "run",argv[2]);
  } else {
    s->readMemoryDump(argv[1]);
  }
  shell(s);
  // FixMe: test doesn't work
  // test();
}
