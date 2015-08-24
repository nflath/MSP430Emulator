// http://en.wikipedia.org/wiki/TI_MSP430#MSP430_CPU
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <assert.h>
#include "source.h"
#include "dest.h"
#include "state.h"
#include "instruction.h"
#include "util.h"

extern State* s;

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

bool printOnAll = false;
bool traceOnAll = false;
bool break_on_print = false;
bool break_on_input = false;
unsigned short print_if = 0;
int steps = 0;

void
execShellCommand(State* s, std::string command, std::string args) {
  if(command == "reset-steps") {
    steps = 0;
  } else if(command == "steps") {
    std::cout << "Steps: " << std::dec << steps << std::hex << std::endl;
  } else if(command == "echo") {
    std::cout << args << std::endl;
  } else if(command == "--print") {
    printOnAll = !printOnAll;
    if(args.length()>0) {
      std::stringstream ss(args);
      ss >> std::hex >> print_if;
    }
  } else if(command == "--trace") {
    traceOnAll = !traceOnAll;
  }else if(command == "--break-on-print") {
    break_on_print = !break_on_print;
  } else if(command == "--break-on-input") {
    break_on_input = !break_on_input;
  }else if(command == "load") {
    s->readMemoryDump(args);
  } else if(command == "dump") {
    s->createMemoryDump();
  } else if(command == "list") {
    s->list();
  } else if(command == "rand") {
    std::stringstream ss(args);
    ss >> std::hex >> s->data.rand;
    s->data.use_rand = true;
  } else if(command == "reset") {
    s->reset();
  } else if(command == "compare") {
    s->compare(args);
  } else if(command == "break" || command == "br") {
    std::stringstream ss(args);
    unsigned short addr;
    ss >> std::hex >> addr;
    s->breakpoint[addr] = true;
  } else if(command == "remove" || command == "rb" || command == "dis") {
    if(args[0] == 'r') {
      s->watchpointRegister.erase(strToR(args));
    } else {
      std::stringstream ss(args);
      unsigned short addr;
      ss >> std::hex >> addr;
      s->breakpoint.erase(addr);
      s->watchpoint.erase(addr);
    }
  } else if(command == "watch") {
    if(args[0] == 'r') {

      s->watchpointRegister[strToR(args)] = s->data.r[strToR(args)];

    } else {
      std::stringstream ss(args);
      unsigned short addr;
      ss >> std::hex >> addr;
      s->watchpoint[addr] = s->data.memory[addr];
    }
  } else if(command == "c" || command == "continue") {
    int numSteps = 1;
    s->data.printed = false;
    for(int i = 0; i < numSteps; i++) {
      while(s->data.running &&
            s->data.locked &&
            !s->watchpoint_triggered&&
            (!break_on_print||!s->data.printed)&&
            (!break_on_input||!s->data.inputed)) {
        steps++;
        s->step();
        if((printOnAll&&!print_if)||(s->data.r[0]==print_if)) {
          std::cout << std::hex << s->data.r[0] << ": "<< std::flush;
          Instruction* j = s->instructionForAddr(s->data.r[0]);
          std::cout << j->toString() << std::endl;
        }
        if(traceOnAll) {
          Instruction* j = s->instructionForAddr(s->data.r[0]);
          std::cout << j->byteStr() << std::endl;
        }
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
      steps++;
      s->step();
      if(s->breakpoint[s->data.r[0]]) {
        break;
      }
    }
    if(s->data.running &&
       (s->readWord(s->data.r[0]) == 0x4130)) {
      steps++;
      s->step();
    }
    if(s->data.running) {
      Instruction* i = s->instructionForAddr(s->data.r[0]);
      std::cout << std::hex << s->data.r[0] << ": " << i->toString() << std::endl;
    }
  } else if(command == "s" || command == "step") {
    if(args.length() > 1) {
      bool print = false;
      std::stringstream ss(args);
      int max;
      ss >> max;
      for(int i = 0; i < max; i++) {
        s->step();
        steps++;
        if((printOnAll&&!print_if)||(s->data.r[0]==print_if)) {
          std::cout << std::hex << s->data.r[0] << ": "<< std::flush;
          Instruction* j = s->instructionForAddr(s->data.r[0]);
          std::cout << j->toString() << std::endl;
        }
      }
    } else {
      s->step();
      steps++;
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
        steps--;
        s->reverse();
      }
    } else {
      steps--;
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
  } else if(command == "exit") {
    exit(0);
  } else if(command == "read-break") {
    std::stringstream ss(args);
    unsigned short addr;
    ss >> std::hex >> addr;
    s->read_breakpoint[addr] = true;
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
}
