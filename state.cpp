#include "state.h"
#include "instruction.h"
#include "util.h"
#include <assert.h>
#include <fstream>

unsigned short
State::readWord(unsigned short addr, bool byte_) {
  if(!byte_ && (addr%2) != 0) {
    std::cout << "load address unaligned: " << std::hex << addr << std::endl;
    if(exit_on_finished) {
      exit(1);
    }
    data.running = false;
  }
  read[read_idx++] = addr;
  read[read_idx++] = addr + 1;

  unsigned short nibble[] = {
    (data.memory[addr+1]&0xf0),
    (data.memory[addr+1]&0x0f),
    (data.memory[addr]&0xf0),
    (data.memory[addr]&0x0f)
  };
  return(nibble[0] << 8) + (nibble[1] << 8) +
    (nibble[2]) + (nibble[3]);
}

unsigned char
State::readByte(unsigned short addr) {
  read[read_idx++] = addr;
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
  Instruction* retn = realInstructionForAddr(addr);
  if(retn) {
    // Possible memcpy
    // addr: MOV @r_x+, 0x0000(r_y)
    // ADD #0002, r_y
    // SUB #0002, r_z
    // CMP #0000, rz
    // JNE addr
    {
      MOV* mov = dynamic_cast<MOV*>(retn);
      if(mov) {
        ADD* add = dynamic_cast<ADD*>(realInstructionForAddr(addr+mov->size()));
        if(add) {
          SUB* sub = dynamic_cast<SUB*>(realInstructionForAddr(addr+add->size()+mov->size()));
          if(sub) {
            CMP* cmp = dynamic_cast<CMP*>(realInstructionForAddr(addr+add->size()+sub->size()+mov->size()));
            if(cmp) {
              JNE* jne = dynamic_cast<JNE*>(realInstructionForAddr(addr+add->size()+sub->size()+cmp->size()+mov->size()));
              if(jne) {
                RegisterSource* movSource = dynamic_cast<RegisterIndirectAutoincrementSource*>(mov->source);
                Constant* addSource = dynamic_cast<Constant*>(add->source);
                Constant* subSource = dynamic_cast<Constant*>(sub->source);
                Constant* cmpSource = dynamic_cast<Constant*>(cmp->source);

                RegisterOffsetDest* movDest = dynamic_cast<RegisterOffsetDest*>(mov->dest);
                RegisterDest* addDest = dynamic_cast<RegisterDest*>(mov->dest);
                RegisterDest* subDest = dynamic_cast<RegisterDest*>(sub->dest);
                RegisterDest* cmpDest = dynamic_cast<RegisterDest*>(cmp->dest);

                if(movSource&&addSource&&subSource&&cmpSource&&movDest&&addDest&&subDest&&cmpDest) {
                  unsigned short source = movSource->reg;
                  unsigned short dest = movDest->reg;
                  unsigned short amount = subDest->reg;
                  if(dest == addDest->reg &&
                     amount == cmpDest->reg&&
                     addSource->val == 2&&
                     subSource->val == 2&&
                     cmpSource->val == 0&&
                     addr == (jne->addr+2*jne->offset+2)) {
                    return new MemcpyModify(dest,source,amount,mov->size()+add->size()+sub->size()+cmp->size()+jne->size());
                  }
                }
              }
            }
          }
        }
      }
    }

    // Possibl memclear
    // 50e8: 8e43 0000: MOV  #0x0000, 0x0000(r14)
    // 50ec: 2e53: ADD   #0x0002, r14
    // 50ee: 2c83: SUB   #0x0002, r12
    // 50f0: 0c93: CMP #0x0000, r12
    // 50f2: fa23: JNE	0x50e8
    {
      MOV* mov = dynamic_cast<MOV*>(retn);
      if(mov) {
        ADD* add = dynamic_cast<ADD*>(realInstructionForAddr(addr+mov->size()));
        if(add) {
          SUB* sub = dynamic_cast<SUB*>(realInstructionForAddr(addr+add->size()+mov->size()));
          if(sub) {
            CMP* cmp = dynamic_cast<CMP*>(realInstructionForAddr(addr+add->size()+sub->size()+mov->size()));
            if(cmp) {
              JNE* jne = dynamic_cast<JNE*>(realInstructionForAddr(addr+add->size()+sub->size()+cmp->size()+mov->size()));
              if(jne) {
                Constant* movSource = dynamic_cast<Constant*>(mov->source);
                Constant* addSource = dynamic_cast<Constant*>(add->source);
                Constant* subSource = dynamic_cast<Constant*>(sub->source);
                Constant* cmpSource = dynamic_cast<Constant*>(cmp->source);

                RegisterOffsetDest* movDest = dynamic_cast<RegisterOffsetDest*>(mov->dest);
                RegisterDest* addDest = dynamic_cast<RegisterDest*>(mov->dest);
                RegisterDest* subDest = dynamic_cast<RegisterDest*>(sub->dest);
                RegisterDest* cmpDest = dynamic_cast<RegisterDest*>(cmp->dest);

                if(movSource&&addSource&&subSource&&cmpSource&&movDest&&addDest&&subDest&&cmpDest) {
                  unsigned short dest = movDest->reg;
                  unsigned short amount = subDest->reg;
                  if(dest == addDest->reg &&
                     amount == cmpDest->reg&&
                     movSource->val == 0 &&
                     addSource->val == 2&&
                     subSource->val == 2&&
                     cmpSource->val == 0&&
                     addr == (jne->addr+2*jne->offset+2)) {
                    return new MemclearModify(dest,amount,mov->size()+add->size()+sub->size()+cmp->size()+jne->size());
                  }
                }
              }
            }
          }
        }
      }
    }

    // Possible r = 0
    {
      ADD* add = dynamic_cast<ADD*>(retn);
      if(add) {
        JNE* jne = dynamic_cast<JNE*>(realInstructionForAddr(addr+add->size()));
        if(jne) {
          Constant* addSource = dynamic_cast<Constant*>(add->source);
          RegisterDest* addDest = dynamic_cast<RegisterDest*>(add->dest);
          if(addSource &&
             addDest &&
             addSource->val == 0xffff &&
             addr == (jne->addr+2*jne->offset+2)) {
            return new Clear(addDest->reg,add->size()+jne->size());
          }
        }
      }
    }

    // Possible decrypt_and_copy?
    do {
      // This is a very esoteric virtual instruction used in the hollywood level
      std::vector<Instruction*> instructions;
      Instruction* i = 0;
      unsigned short addr = addr;

      i = dynamic_cast<MOV*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<PUSH*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      JMP* jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!jmp) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!jmp) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<MOV*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<SWPB*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!jmp) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      SUB* sub = dynamic_cast<SUB*>(realInstructionForAddr(addr));
      if(!sub) { break; }
      instructions.push_back(sub);
      addr += sub->size();

      RegisterDest* subDest = dynamic_cast<RegisterDest*>(sub->dest);
      Constant* subSource = dynamic_cast<Constant*>(sub->source);
      if(!subDest || ! subSource || subDest->reg != 14 || subSource->val != 0x04d2) { break; }

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!jmp) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      // FixMe: Stop checking registers/values

      i = dynamic_cast<ADD*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<RRC*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!jmp) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<DADD*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!jmp) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<RRA*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<RRC*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<DADD*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<ADD*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<RRC*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<ADD*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<RRC*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<RRC*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<XOR*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<MOV*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      i = dynamic_cast<ADD*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);
      addr += i->size();

      jmp = dynamic_cast<JMP*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(jmp);
      addr = jmp->addr+2*jmp->offset+2;

      i = dynamic_cast<MOV*>(realInstructionForAddr(addr));
      if(!i) { break; }
      instructions.push_back(i);

      addr = s->data.r[0] + 0x51ec-0x51e8 + 2;

      MOV* mov = dynamic_cast<MOV*>(realInstructionForAddr(addr));
      if(!mov) { break; }
      instructions.push_back(mov);
      addr += mov->size();
      RegisterOffsetDest* dest = dynamic_cast<RegisterOffsetDest*>(mov->dest);

      InstructionList* il = new InstructionList("DecryptAndCopy");
      il->instructions = instructions;
      std::stringstream ss;

      ss << "DecryptAndCopy r10, " << mov->dest->toString() << " (0x" << std::hex << data.r[10];
      ss << ", 0x";
      ss << data.r[12]+dest->offset;
      ss << ")";
      il->toString_ = ss.str();
      return il;

    } while(false);

    do {
      // Possible memclear
      std::vector<Instruction*> instructions;
      Instruction* i = 0;

      std::stringstream ss;
      unsigned short lastOffset = 0;
      unsigned short lastAddr = 0;
      unsigned short addr_ = addr;

      short r = -1;
      while(addr_ != lastAddr) {

        lastAddr = addr_;

        MOV* mov = dynamic_cast<MOV*>(realInstructionForAddr(addr_));
        if(!mov) { break; }

        Constant* source = dynamic_cast<Constant*>(mov->source);
        RegisterOffsetDest* dest = dynamic_cast<RegisterOffsetDest*>(mov->dest);

        if(!source||!dest) { break; }
        if(!(r == -1 || dest->reg == r)) { break; }
        if(source->value()) { break; }
        if(lastOffset && lastOffset != (dest->offset-2)) { break; }

        r = dest->reg;
        instructions.push_back(mov);
        if(ss.str()=="") {
          ss << "memclear(0x" << std::hex << dest->offset << "(r" << std::dec << dest->reg << "), ";
        }

        addr_ = addr_ + mov->size();

        lastOffset = dest->offset;
      }

      if(instructions.size()>1) {
        InstructionList* il = new InstructionList("memclear");
        il->instructions = instructions;
        ss << std::dec << instructions.size()*2 << ")";
        il->toString_ = ss.str();
        return il;
      }

    } while(false);

    do {
      // Possible r15=rand()
      unsigned short addr_=addr;
      std::vector<Instruction*> instructions;

      MOV* mov = dynamic_cast<MOV*>(realInstructionForAddr(addr_));
      if(!mov) { break; }
      Constant* source = dynamic_cast<Constant*>(mov->source);
      RegisterDest* dest = dynamic_cast<RegisterDest*>(mov->dest);
      if(!source||source->val!=0xa000) { break; }
      if(!dest||dest->reg!=2) {break; }
      instructions.push_back(mov);
      addr_ += mov->size();

      CALL* call = dynamic_cast<CALL*>(realInstructionForAddr(addr_));
      if(!call) { break; }
      source = dynamic_cast<Constant*>(call->source);
      if(!source||source->value()!=0x10) { break; }
      instructions.push_back(call);
      instructions.push_back(new INTERRUPT());
      addr_ += call->size();

      ADD* add = dynamic_cast<ADD*>(realInstructionForAddr(addr_));
      if(!add) { break; }
      source = dynamic_cast<Constant*>(add->source);
      dest = dynamic_cast<RegisterDest*>(add->dest);
      if(!source||source->value()) { break; }
      instructions.push_back(add);

      InstructionList* il = new InstructionList("r15=rand()");
      il->instructions = instructions;
      std::stringstream ss;
      ss << "r" << std::dec << dest->reg << " = rand()";
      il->toString_ = ss.str();

      return il;
    } while(false);

  }
  return retn;
}

Instruction*
State::realInstructionForAddr(unsigned short addr) {
  // See Instruction.h for a list of references about the MSP430 instruction
  // set
  unsigned short val = readWord(addr);
  Instruction * retn_ = 0;
  if(addr == 0x10) {
    retn_ = new INTERRUPT();
  } else if((val&0xff00) == 0) {
    InstructionOneOperand* retn = new RRC();
    unsigned short bw = (val&0x0040) >> 6;
    unsigned short as = (val&0x0030) >> 4;
    unsigned short reg = (val&0x000f);
    retn->byte = bw;
    retn->source = sourceOperand(as, reg, addr+2);
    if(retn->source) {
      retn_ = retn;
    }
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
    if(retn->source) {
      retn_ = retn;
    }
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
    retn_ = retn;
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
    if(retn->source) {

      if(retn->source->usedExtensionWord) {
        retn->dest = destOperand(ad, dest, addr + 4);
      } else {
        retn->dest = destOperand(ad, dest, addr + 2);
      }

      if(retn->dest) {
        retn_ = retn;
      }
    }


  }
  if(!retn_) {
    return 0;
  } else {
    int i = 0;
    int s = retn_->size();
    while(s) {
      retn_->bytes[i] = (data.memory[addr+i*2] << 8) + data.memory[addr+i*2+1];
      i++;
      s-=2;
    }
    return retn_;
  }
}


Source*
State::sourceOperand(unsigned short as, unsigned short source, unsigned short addr) {
  if(source == 0) {
    switch(as) {
    case 0: {
      return new RegisterSource(source,(unsigned char*)&s->data.r[source]);
    }
    case 1: {
      return new RegisterIndirectSource(0,&s->data.memory[s->data.r[source]]);
    }
    case 2: {
      return new RegisterIndirectSource(0,&s->data.memory[s->data.r[source]]);
    }
    case 3: {
      return new Constant(readWord(addr),true);
    }
    }
  } else if(source == 2) {
    switch(as) {
    case 0: {
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
      return new RegisterDest(dest);
    }
    case 1: {
      return new RegisterOffsetDest(0,readWord(addr));
    }
    }
  } else if(dest == 2) {
    switch(ad) {
    case 0: {
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
  lastDataFile =filename;
  std::ifstream f(filename.c_str());
  if (f.is_open()) {
    std::string line;
    for(unsigned short i = 0; i < 0xffff; i++) {
      data.memory[i] = 0;
    }
    data.r[0] = 0x4400;
    for(int i = 1; i < 15; i++) {
      data.r[i] = 0;
    }

    while ( getline (f,line) ) {
      try {
        if(line == "") {
          continue;
        }
        if(std::string(line.substr(0,2)) == "pc" || line[0] == 'r') {
          for(int i = 0; i < 4; i++) {
            std::string reg = line.substr(i * 10,3);
            std::stringstream ss;
            unsigned short value;
            ss << std::hex << line.substr(i*10+4,i*10+8);
            ss >> value;
            data.r[strToR(reg)] = value;
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
      } catch(...) {
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
  for(unsigned int i = 0; i <= 0xfff; i++) {

    std::stringstream ss;
    for(unsigned int j = 0; j < 0x8; j++) {
      ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)data.memory[(i<<4) + j*2]
         << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)data.memory[(i<<4)+(j*2+1)] << " ";
    }
    if(ss.str() == "0000 0000 0000 0000 0000 0000 0000 0000 ") {
      if(starPrinted) {
      } else {
        std::cout << std::hex << std::setw(3) << std::setfill('0') << i << "0:   *" << std::endl;
        starPrinted = true;
      }
    } else {
      std::cout << std::hex << std::setw(3) << std::setfill('0') << i << "0:   ";
      std::cout << ss.str();
      std::cout << std::endl;
      starPrinted = false;
    }

  }
  for(unsigned int r = 0; r < 16; r++) {
    std::cout << "r" << std::setbase(10) << std::setw(2) << std::setfill('0') << r << " " << std::hex << std::setw(4) << std::setfill('0') << data.r[r];
    if(r%4==3) {
      std::cout << std::endl;
    } else {
      std::cout << "  ";
    }
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
  data.inputed = false;
  read_idx = 0;
  if((data.r[0] % 2)) {
    data.running = false;
    std::cout << "ISN unaligned:" << std::hex << data.r[0] << std::endl;
    return;
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

  if(data.r[2]&0x0010) {
    data.running = false;
    if(exit_on_finished) {
      exit(1);
    }
  }

  delete i;

  for(int i = 0; i < read_idx; i++) {
    if(read_breakpoint.count(read[i])) {
      std::cout << "Read watchpoint triggered: 0x" << std::hex << read[i] << std::endl;
      watchpoint_triggered = true;
    }
  }

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
