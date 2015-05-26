#include "state.h"
#include "instruction.h"
#include <assert.h>
#include <fstream>

unsigned short
State::readWord(unsigned short addr, bool byte_) {
  if(!byte_ && (addr%2) != 0) {
    std::cout << "load address unaligned: " << std::hex << addr << std::endl;
    data.running = false;
  }
  unsigned short nibble[] = {
    (data.memory[addr+1]&0xf0),// >> 4,
    (data.memory[addr+1]&0x0f),// << 4,
    (data.memory[addr]&0xf0),// >> 4,
    (data.memory[addr]&0x0f)// << 4
  };
  return(nibble[0] << 8) + (nibble[1] << 8) +
    (nibble[2]) + (nibble[3]);

  //return(nibble[0]) + (nibble[1]) +
  //(nibble[2] << 8) + (nibble[3] << 8);
}

unsigned char
State::readByte(unsigned short addr) {
  return data.memory[addr];
}


void
State::reset(bool rereadFile) {
  data.locked = true;
  data.running = true;
  input_idx = 0;
  data_idx = 0;
  data.dep_on = false;
  watchpoint_triggered=false;



  if(rereadFile) {
    if(lastDataFile!="") {
      readMemoryDump(lastDataFile);
    }
  }
  for(std::map<unsigned short, unsigned char>::iterator j =  watchpoint.begin(); j != watchpoint.end(); j++) {
    watchpoint[j->first] = data.memory[j->first];
  }
}

Instruction*
State::instructionForAddr(unsigned short addr) {
  unsigned short val = readWord(addr);
  if(addr == 0x10) {
    return new INTERRUPT();
  } else if((val&0xff00) == 0) {
    InstructionOneOperand* retn = new RRC();
    unsigned short bw = (val&0x0040) >> 6;
    unsigned short as = (val&0x0030) >> 4;
    unsigned short reg = (val&0x000f);
    retn->byte = bw;
    retn->source = sourceOperand(as, reg, addr+2);
    return retn;
  } else if( (val&0xfb00) >> 10 == 0b000100) {
    OpCode_OneOperand opcode = (OpCode_OneOperand)((val&0x0380) >> 7);
    unsigned short bw = (val&0x0040) >> 6;
    unsigned short as = (val&0x0030) >> 4;
    unsigned short reg = (val&0x000f);

    InstructionOneOperand* retn = 0;
    switch(opcode) {
    case RRC_: retn = new RRC(); break;
    case SWPB_: retn = new SWPB(); break;
    case RRA_: retn = new RRA(); break;
    case SXT_: retn = new SXT(); break;
    case PUSH_: retn = new PUSH(); break;
    case CALL_: retn = new CALL(); break;
    case RETI_: retn = new RETI(); break;
    default: return 0;
    }
    if(opcode != RETI_) {
      retn->byte = bw;
      retn->source = sourceOperand(as, reg, addr+2);
    }
    return retn;
  } else if( (val&0xe000) >> 13 == 0b001 ) {

    ConditionCode condition = (ConditionCode)((val&0x1c00)>>10);
    short offset = (val&0x03ff);
    if((offset&0x200)!=0) {
      offset |= 0xfc00;
    }

    Condition* retn = 0;
    switch(condition) {
    case JNE_: retn = new JNE(); break;
    case JEQ_: retn = new JEQ(); break;
    case JNC_: retn = new JNC(); break;
    case JC_: retn = new JC(); break;
    case JN_: retn = new JN(); break;
    case JGE_: retn = new JGE(); break;
    case JL_: retn = new JL(); break;
    case JMP_: retn = new JMP(); break;
    default: return 0;
    }

    retn->addr = addr;
    retn->offset = offset;
    return retn;
  } else {
    OpCode_TwoOperand opcode = (OpCode_TwoOperand)((val&0xf000) >> 12);
    unsigned short source = (val&0x0f00) >> 8;
    unsigned short ad = (val&0b0000000010000000) >> 7;
    unsigned short bw = (val&0b0000000001000000) >> 6;
    unsigned short as = (val&0b0000000000110000) >> 4;
    unsigned short dest = val&0x000f;

    InstructionTwoOperands* retn = 0;
    switch(opcode) {
    case MOV_:retn = new MOV(); break;
    case ADD_:retn = new ADD(); break;
    case ADDC_:retn = new ADDC(); break;
    case SUBC_:retn = new SUBC(); break;
    case SUB_:retn = new SUB(); break;
    case CMP_:retn = new CMP(); break;
    case DADD_:retn = new DADD(); break;
    case BIT_:retn = new BIT(); break;
    case BIC_:retn = new BIC(); break;
    case BIS_:retn = new BIS(); break;
    case XOR_:retn = new XOR(); break;
    case AND_:retn = new AND(); break;
    default: return 0;
    }
    retn->byte = bw;
    retn->source = sourceOperand(as, source, addr+2);
    if(retn->source->usedExtensionWord) {
      retn->dest = destOperand(ad, dest, addr + 4);
    } else {
      retn->dest = destOperand(ad, dest, addr + 2);
    }

    return retn;
  }
  return 0;
}


Source*
State::sourceOperand(unsigned short as, unsigned short source, unsigned short addr) {
  if(source == 0) {
    // Using the PC
    switch(as) {
    case 0: {
      return new RegisterSource(source,(unsigned char*)&s->data.r[source]); // FixMe is this correct?
    }
    case 1: {
      return new RegisterIndirectSource(0,&s->data.memory[s->data.r[source]]);
    }
    case 2: {
      return new RegisterIndirectSource(0,&s->data.memory[s->data.r[source]]);
    }
    case 3: {
      return new Constant(readWord(addr),true);
      //new RegisterIndirectAutoincrementSource(0); // FixMe: Is this true?
    }
    }
  } else if(source == 2) {
    switch(as) {
    case 0: {
      // FixMe: is this correct?
      return new RegisterSource(source,(unsigned char*)&s->data.r[source]);
    }
    case 1: {
      return new Absolute(readWord(addr));
    }
    case 2: {
      return new Constant(4,false);
    }
    case 3: {
      return new Constant(8,false);
    }
    }
  } else if(source == 3) {
    switch(as) {
    case 0: {
      return new Constant(0,false);

      case 1: {
        return new Constant(1,false);
      }
      case 2: {
        return new Constant(2,false);
      }
      case 3: {
        return new Constant(-1,false);
      }
    }
    }
  } else {
    switch(as) {
    case 0 : {
      return new RegisterSource(source,(unsigned char*)&s->data.r[source]);
    }
    case 1 : {
      return new RegisterIndexedSource(source, readWord(addr));
    }
    case 2 : {
      return new RegisterIndirectSource(source,&s->data.memory[s->data.r[source]]);
    }
    case 3 : {
      return new RegisterIndirectAutoincrementSource(source,&s->data.memory[s->data.r[source]]);
    }
    }
  }
  return 0;
}

Dest*
State::destOperand(unsigned short ad, unsigned short dest, unsigned short addr) {
  if(dest == 0) {
    switch(ad) {
    case 0: {
      // FixMe: Is this correct?
      return new RegisterDest(dest);
    }
    case 1: {
      return new RegisterOffsetDest(0,readWord(addr));
    }
    }
  } else if(dest == 2) {
    switch(ad) {
    case 0: {
      // FixMe: Is this correct?
      return new RegisterDest(dest);
    }
    case 1: {
      return new AbsoluteDest(readWord(addr));
    }
    }
  } else {
    switch(ad) {
    case 0 : {
      return new RegisterDest(dest);
    }
    case 1 : {
      return new RegisterOffsetDest(dest,readWord(addr));
    }
    }
  }
  return 0;
}

unsigned short
State::strToReg(std::string str) {
  if(str.substr(0,2) == "pc") {
    return 0;
  } if(str.substr(0,2) == "sp") {
    return 1;
  } if(str.substr(0,2) == "sr") {
    return 2;
  } if(str.substr(0,2) == "cg") {
    return 3;
  }
  std::stringstream ss;
  ss << str.substr(1,str.size());
  unsigned short r;
  ss >> r;
  return r;
}

void
State::compare(std::string filename) {
  State* s2 = new State();
  s2->readMemoryDump(filename);
  for(unsigned short i = 0; i < sizeof(data.memory); i++) {
    if(data.memory[i] != s2->data.memory[i]) {
      std::cout << "Difference at " << std::hex << "0x" << i << ": "
                << (unsigned short)data.memory[i]
                << " vs " << (unsigned short)s2->data.memory[i] << std::endl;
    }
  }
  for(unsigned short i = 0; i < 16; i++) {
    if(data.r[i] != s2->data.r[i]) {
      std::cout << "Difference in r" << i << ": " << std::hex
                << (unsigned short)data.r[i] << " vs "
                << (unsigned short)s2->data.r[i] << std::endl;
    }
  }
  delete s2;
}

void
State::readMemoryDump(std::string filename) {
  lastFile = "";
  lastDataFile =filename;
  std::ifstream f(filename.c_str());
  if (f.is_open()) {
    std::string line;
    bool resetRegisters = true;
    for(unsigned short i = 0; i < 0xffff; i++) {
      data.memory[i] = 0;
    }

    while ( getline (f,line) ) {
      // 4 cases: instruction, label, string, section

      if(line == "") {
        continue;
      }
      if(std::string(line.substr(0,2)) == "pc" || line[0] == 'r') {
        for(int i = 0; i < 4; i++) {
          resetRegisters = false;
          std::string reg = line.substr(i * 10,3);
          std::stringstream ss;
          unsigned short value;
          ss << std::hex << line.substr(i*10+4,i*10+8);
          ss >> value;
          data.r[strToReg(reg)] = value;
        }
      } else if(line.substr(0,2)!="--") {
        std::string addrString = line.substr(0,4);
        std::stringstream ss;
        unsigned short addr;
        ss << std::hex << addrString;
        ss >> addr;

        if(line[8] == '*') {
          continue;
        } else {
          for(int i = 0; i < 16; i++) {
            data.memory[addr+i] = (unsigned char)(strtoul(line.substr(8+i*2+i/2,2).c_str(), NULL, 16));
          }
        }
      }
    }

    if(resetRegisters) {
      data.r[0] = 0x4400;
      for(int i = 1; i < 15; i++) {
        data.r[i] = 0;
      }
    }

  } else {
    std::cout << "Error opening file: '" << filename << "'" << std::endl;
  }
  reset(false);
}

void
State::createMemoryDump() {
  bool starPrinted = false;
  for(unsigned int i = 0; i < 0xfff; i++) {

    std::stringstream ss;
    for(unsigned int j = 0; j < 0x8; j++) {
      ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)data.memory[(i<<4) + j]
         << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)data.memory[(i<<4)+(j*2+1)] << " ";
    }
    if(ss.str() == "0000 0000 0000 0000 0000 0000 0000 0000 ") {
      if(starPrinted) {
      } else {
        std::cout << std::hex << i << "0:   *" << std::endl;
        starPrinted = true;
      }
    } else {
      std::cout << std::hex << i << "0:   ";
      std::cout << ss.str();
      std::cout << std::endl;
      starPrinted = false;
    }

  }
  for(unsigned int r = 0; r < 16; r++) {
    std::cout << "r" << std::setbase(10) << r << " " << std::hex << std::setw(4) << std::setfill('0') << data.r[r] << std::endl;
  }
}

void
State::list() {
  int i = 0x10;
  std::string currentSection = "";
  while(i != 0xffff) {
    if(label.find((unsigned short)i) != label.end()) {
      std::cout << std::hex << std::setw(4) << std::setfill('0') << i << " <" << label[i] << ">" << std::endl;
      currentSection = "";
    }
    if(section.find((unsigned short)i) != section.end()) {
      std::cout << std::hex << std::setw(4) << std::setfill('0') << i << " ." << section[i] << ":" << std::endl;
      currentSection = section[i];
    }
    if(data.memory[i] == 0 && data.memory[i+1] == 0x13) {
      return;
    }
    if(data.memory[i] == 0) {
      i++;
      continue;
    }
    if(currentSection!="") {
      i++;
      continue;
    }
    Instruction* instruction = instructionForAddr((unsigned short)i);

    std::cout << std::hex << std::setw(4) << std::setfill('0') << i << ": ";

    for(int j = 0; j < instruction->size()/2; j++) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)data.memory[i+2*j] <<  std::hex << std::setw(2) << std::setfill('0') << (unsigned short)data.memory[i+2*j+1] << " ";
    }
    i += instruction->size();

    std::cout << instruction->toString() << std::endl;


    //delete instruction;
  }
}

void
State::reverse() {
  if(data_idx == 0) {
    data_idx = 100;
  }
  data_idx = (data_idx - 1 ) %  100;

  memcpy(&data, &prev_data[data_idx], sizeof(data));
}

void
State::writeByte(unsigned short addr, unsigned char value) {
  if(data.dep_on) {
    unsigned short page = addr / 0x100;
    assert(data.page_writable[page]);
  }
  data.memory[addr] = value;
}

void
State::step() {
  watchpoint_triggered = false;
  data.printed = false;
  if((data.r[0] % 2)) {
    data.running = false;
    std::cout << "ISN unaligned:" << std::hex << data.r[0] << std::endl;
  }

  memcpy(&prev_data[data_idx],&data, sizeof(data));
  data_idx = (data_idx + 1 ) % 100;

  Instruction* i = instructionForAddr(data.r[0]);

  int pc = data.r[0];
  if(!i) {
    data.running = false;
    std::cout << "Invalid instruction at: " << std::hex << data.r[0] << " with data.memory 0x" << std::hex << readWord(data.r[0]) << std::endl;
    OpCode_TwoOperand opcode = (OpCode_TwoOperand)((readWord(data.r[0])&0xf000) >> 12);
    std::cout << "Opcode: " << opcode << std::endl;
    return;
  }


  if(data.r[0]==pc) {
    data.r[0] += i->size();
  }


  i->execute(this);


  if(!data.locked) {
    if(exit_on_finished) {
      exit(0);
    }
  }
  if(data.r[2]&0x0080) {
    data.running = false;
    if(exit_on_finished) {
      exit(1);
    }
  }

  delete i;

  for(std::map<unsigned short, unsigned char>::iterator j =  watchpoint.begin(); j != watchpoint.end(); j++) {
    if(j->second != data.memory[j->first]) {
      std::cout << "Watchpoint triggered: " << j->first << std::endl;
      watchpoint[j->first] = data.memory[j->first];
      watchpoint_triggered = true;
    }
  }

  for(std::map<unsigned char, unsigned short>::iterator j =  watchpointRegister.begin(); j != watchpointRegister.end(); j++) {
    if(j->second != data.r[j->first]) {
      std::cout << "Watchpoint triggered: r" << j->first << std::endl;
      watchpointRegister[j->first] = data.r[j->first];
      watchpoint_triggered = true;
    }
  }
}

State* s = new State();
