#include "instruction.h"
#include "state.h"
#include "util.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

void
setandflags(State*s, unsigned short value, bool byte=false) {
  // Utility function to set flags for one-operand operations
  s->data.r[2] =
    (byte?(!!(value&0x8000) << 2):(!!(value&0x80) << 2)) |
    !value << 1 |
    !!value;
}


void
setflags(unsigned int result, bool bw, State* s) {
  // Used to set status flags for SUB/ADD instruction.
  unsigned short sr = 0;
  unsigned sz = 16;
  if(bw) {
    sz = 8;
  }

  if(bw == 0 && (result & 0x8000))  {
    sr |= 0x4;
  }

  if ((result & ((1 << sz) - 1)) == 0) {
    sr |= 0x2;
  }

  if (result & (1 << sz)) {
    sr |= 0x1;
  }

  s->data.r[2] = sr;
}

std::string
Condition::toString() {
  std::stringstream ss,ss2;
  ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << (addr+2+offset*2);
  return instructionName() + "\t" + ss.str();
}

std::string InstructionOneOperand::toString() {
  return instructionName() + (byte ? ".b\t" : "\t") + source->toString();
}

std::string InstructionTwoOperands::toString() {
  std::string str = instructionName() + (byte ? ".b\t" : "\t") + source->toString() + ", " + dest->toString();
  if(str == "MOV\t@sp+, PC") {
    return "RET";
  }
  return str;
}

void
Instruction::execute(State* s) {
  notimplemented();
}

std::string
INTERRUPT::toString() {
  return "INTERRUPT";
}

void
INTERRUPT::execute(State* s) {
  // Perform an interrupt based on what is in the status register.
  unsigned char interrupt = (s->data.r[2]&0x7f00) >> 8;
  switch(interrupt) {
  case 0x00: {// putchar
    char c = (char)s->data.memory[s->data.r[1]+8];
    if((c >= 10 && c < 127)) {
      std::cout << c << std::flush;
    }
    s->data.printed = true;
    break;
  }
  case 1: { // getchar
    std::cout << std::endl << "Input byte: ";
    unsigned short destaddr = s->readWord(s->data.r[1]+8);

    std::string str = readString();
    const char* c_str = str.c_str();

    char byte;

    assert(strlen(c_str)==2);

    int i = 0;
    byte = c_str[i] > '9' ? (c_str[i] - 'a' + 10) : (c_str[i] - '0');
    byte = (byte << 4) +  (c_str[i+1] > '9' ? (c_str[i+1] - 'a' + 10) : (c_str[i+1] - '0'));

    s->writeByte(destaddr,byte);

    break;
  }
  case 0x02: { // gets
    unsigned short destaddr = s->readWord(s->data.r[1]+8);
    unsigned short maxbytes = s->readWord(s->data.r[1]+10);
    std::string str;
    if(s->input.size()) {
      str = s->input.front();
      s->input.erase(s->input.begin());
    } else {
      str = readString();
    }

    const char* c_str = str.c_str();

    char byte;
    int i = 0;

    for(i = 0; (i < (maxbytes*2)) && (i < strlen(c_str)); i+=2) {
      byte = c_str[i] > '9' ? (c_str[i] - 'a' + 10) : (c_str[i] - '0');
      byte = (byte << 4) +  (c_str[i+1] > '9' ? (c_str[i+1] - 'a' + 10) : (c_str[i+1] - '0'));
      s->writeByte(destaddr, byte);
      destaddr++;
    }

    s->data.inputed = true;
    break;
  }
  case 0x10: { // DEP
    s->data.dep_on = true;
    break;
  }
  case 0x11: { // either executable or writable
    unsigned short page = s->readByte(s->data.r[1]+8);
    unsigned short writable = s->readByte(s->data.r[1]+10);
    if(writable > 1) {
      std::cout << "Bad value for writable:" << writable << std::endl;
      s->data.running = false;
      return;
    }
    assert(page < 256);
    s->data.page_writable[page] = writable;

    break;
  }
  case 0x20: { // rand
    if(s->data.use_rand) {
      s->data.r[15] = s->data.rand;
    } else {
      s->data.r[15] = rand();
    }
    break;
  }
  case 0x7D: { // set flag if password is correct
    unsigned short passaddr = s->readWord(s->data.r[1]+10);
    unsigned short flagaddr = s->readWord(s->data.r[1]+10);
    s->writeByte(flagaddr, 0);
    break;
  }
  case 0x7E: { // unlock if password is correct
    std::cout << std::endl << "Password Failed." << std::endl;
    break;
  }
  case 0x7F: { // unlock
    s->data.locked = false;
    std::cout << std::endl << "Door unlocked!" << std::endl;
    break;
  }
  default:
  std::cout << "Interrupt unknown: " << interrupt << "address: " << std::hex << s->data.r[0] << std::endl;
    notimplemented();
  }
  unsigned short retn = s->readWord(s->data.r[1]);
  s->data.r[1] = s->data.r[1] + 2;
  s->data.r[0] = retn;
  return;
}

std::string
MemcpyModify::toString() {
  std::stringstream ss_x;
  ss_x << source;
  std::stringstream ss_y;
  ss_y << dest;
  std::stringstream ss_z;
  ss_z << amount;
  return "memcpy_modify(r" + ss_y.str() + ", r" + ss_x.str() + ", r" + ss_z.str() + ")";
}

void
MemcpyModify::execute(State* s) {
  if( ((s->data.r[source] > s->data.r[dest]) && (s->data.r[dest]+s->data.r[amount]) > s->data.r[source])
      || ((s->data.r[source] < s->data.r[dest]) && (s->data.r[source]+s->data.r[amount]) > s->data.r[dest])) {
    // Don't support overlapping memory segments
    notimplemented();
  } else {
    memcpy(s->data.memory+s->data.r[dest],
           s->data.memory+s->data.r[source],
           s->data.r[amount]);
    s->data.r[source] = s->data.r[source] + amount;
    s->data.r[dest] = s->data.r[dest] + amount;
    s->data.r[amount] = 0;
  }
}

std::string
Clear::toString() {
  std::stringstream ss;
  ss << r;
  return "CLEAR r"+ss.str();
}

void
Clear::execute(State* s) {
  s->data.r[r] = 0;
}

void
InstructionList::execute(State* s) {
  for(int i = 0; i < instructions.size(); i++) {
    s->data.r[0] += instructions[i]->size();
    instructions[i]->execute(s);
  }
}

std::string
MemclearModify::toString() {
  std::stringstream ss_y;
  ss_y << dest;
  std::stringstream ss_z;
  ss_z << amount;
  return "memclear_modify(r" + ss_y.str() + ", r" + ss_z.str() + ")";
}

void
MemclearModify::execute(State* s) {
  memset(s->data.memory+s->data.r[dest],
         0,
         s->data.r[amount]);
    s->data.r[dest] = s->data.r[dest] + amount;
    s->data.r[amount] = 0;
}

void
JEQ::execute(State* s) {
  if((s->data.r[2]&0x2)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JL::execute(State* s) {
  if((s->data.r[2]&0x4)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JC::execute(State* s) {
  if((s->data.r[2]&0x1)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JN::execute(State* s) {
  if((s->data.r[2]&0x1)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JNC::execute(State* s) {
  if((s->data.r[2]&0x1)==0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JGE::execute(State* s) {
  if((s->data.r[2]&0x4)==0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
SWPB::execute(State* s) {
  unsigned short val = source->value();

  unsigned char b1 = (val & 0xff00) >> 8;
  unsigned char b2 = (val & 0x00ff);

  source->setValue((b2 << 8) + b1);
}

void
JNE::execute(State* s) {
  if((s->data.r[2]&0x2)==0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JMP::execute(State* s) {
  s->data.r[0] = addr+2*offset+2;
}

void
SXT::execute(State* s) {
  if(source->valueByte()&0x80) {
    source->setValue(0xff00 | source->valueByte());
    setandflags(s, source->value());
  }
}

void
CMP::execute(State* s) {
  if(!byte) {
    unsigned int carry = (unsigned int)dest->value() + (~(unsigned int)source->value()+1);
    short result =  dest->value() - source->value();
    s->data.r[2] =
      (((source>0&dest<0 & result<0)|(source<0&dest>0 & result>0)) << 3) +
      ((result < 0) << 2) +
      ((result == 0) << 1) +
      (~carry>>16&1);

  } else {
    unsigned short carry = dest->valueByte() + (~source->valueByte() + 1);
    unsigned char result = dest->valueByte() - source->valueByte();
    s->data.r[2] =
      (((source>0&dest<0 & result<0)|(source<0&dest>0 & result>0)) << 3) +
      ((result < 0) << 2) +
      ((result == 0) << 1) +
      (~carry>>8&1);
  }
}

void
MOV::execute(State* s) {
  if(!byte) {
    dest->set( source->value());
  } else {
    dest->setByte( source->valueByte());
  }
}

void
PUSH::execute(State* s) {
  s->writeByte(s->data.r[1]-1,((unsigned char)((source->value()&0xff00) >> 8)));
  s->writeByte(s->data.r[1]-2,((unsigned char)((source->value()&0x00ff))));
  s->data.r[1] -= 2;
}

void
CALL::execute(State* s) {
  s->writeByte(s->data.r[1]-1,(unsigned char)( ((s->data.r[0])&0xff00) >> 8));
  s->writeByte(s->data.r[1]-2,(unsigned char)( ((s->data.r[0])&0x00ff)));
  s->data.r[1] -= 2;
  s->data.r[0] = source->value();
}

void
AND::execute(State* s) {
  short result = 0;
  if(!byte) {
    result = (dest->value())&(source->value());
    dest->set(result);
  } else {
    result = (dest->value() & 0xff)&(source->value() & 0xff);
    dest->set(result);
  }
  setandflags(s,dest->value(),byte);
}

void
ADD::execute(State* s) {
  unsigned int result =  0;
  if(!byte) {
    unsigned int dv = (0x0000ffff & (unsigned int)dest->value());
    unsigned int sv = (0x0000ffff & (unsigned int)source->value());
    result = dv + sv;

    dest->set(result);
  } else {
    unsigned int dv = (0x000000ff & (unsigned int)dest->valueByte());
    unsigned int sv = (0x000000ff & (unsigned int)source->valueByte());
    result =  dv + sv;

    dest->setByte(result);
    s->data.r[2] = 0x0f00;
  }
  setflags(result, byte, s);
}

unsigned short
convertBcdToHex(unsigned int value) {
  unsigned int result = 0;
  unsigned int multiplier = 1;
  for(int position = 0; position < 4; position++) {
    unsigned int carry = 0;
    unsigned int tmp = value & (0x000f << (position * 4));
    tmp = tmp >> (position * 4);
    if(tmp >= 10) {
      tmp = tmp - 10;
      carry = 1;
    }
    result = result + (tmp * multiplier) + carry * (multiplier *10);
    multiplier*=10;
  }
  return result;
}

unsigned short
convertHexToBcd(unsigned int value) {
  std::cout << "convertHexToBcd: " << value << std::endl;
  unsigned int result = 0;
  if(value >= 10000) {
    value /= 10;
  } else {
    int position = 0;
    for(int multiplier = 10; multiplier < 100000; multiplier *= 10) {
      int tmp = (value % (multiplier)) / (multiplier / 10);
      value = value - (tmp * (multiplier / 10));
      result = result + (tmp << (position * 4));

      position++;
    }
  }
  return result;
}

struct BcdResult {
  unsigned short result;
  unsigned short carry;
  unsigned short setn;
};

BcdResult
addBcd(unsigned int val1, unsigned int val2) {
  unsigned short carry = 0;
  unsigned short result = 0;
  unsigned short setn;
  for(int position = 0; position < 4; position++) {
    unsigned char nibble =
      carry +
      (((val1 & (0xf << position * 4))) >> (position * 4)) +
      (((val2 & (0xf << position * 4))) >> (position * 4));
    setn = !!(nibble&0x8);
    if( nibble > 9 ) {
      nibble = 0xf&(nibble - 10);
      carry = 1;
    } else {
      carry = 0;
    }
    result |= nibble << (position * 4);
  }
  BcdResult bcd;
  bcd.result = result;
  bcd.carry = carry;
  bcd.setn = setn;
  return bcd;
}

void
DADD::execute(State* s) {
  if(!byte) {
    BcdResult bcd = addBcd(dest->value(),source->value());

    dest->set(bcd.result);

    s->data.r[2] =
      ((bcd.setn?bcd.setn:(!!(s->data.r[2]&0x4))) << 2) |
      ((bcd.result == 0) << 1) |
      bcd.carry;
  } else {
    notimplemented();
  }
}

void
SUB::execute(State* s) {
  if(!byte) {
    unsigned int olddest = 0xffff&(unsigned int)dest->value();
    unsigned int oldsource = 0xffff&(unsigned int)source->value();
    unsigned int tmpsrc = 0xffff&(~(unsigned int)oldsource + 1);
    unsigned int result = olddest + tmpsrc;

    int carry = ((result >> 16) & 0x1) || !((unsigned short)source->value() >> 15);

    dest->set(result);

    setflags(result, byte, s);

  } else {
    notimplemented();
  }
}

void
BIC::execute(State* s) {
  if(!byte) {
    unsigned short result = (dest->value())&~(source->value());
    dest->set(result);
  } else {
    notimplemented();
  }
}

void
BIT::execute(State* s) {
  if(!byte) {
    unsigned short result = (dest->value())&(source->value());
    setandflags(s,result);
  } else {
    notimplemented();
  }
}

void
BIS::execute(State* s) {
  if(!byte) {
    dest->set((dest->value())|(source->value()));
  } else {
    notimplemented();
  }
}

void
XOR::execute(State* s) {
  if(!byte) {
    dest->set((dest->value())^(source->value()));
    setandflags(s, dest->value());
  } else {
    dest->setByte((dest->valueByte())^(source->valueByte()));
    setandflags(s, dest->valueByte(),false);
  }
}

void
RRC::execute(State* s) {
  int carry = 0;
  if(!byte) {
    carry = source->value()&0x1;
    if(s->data.r[2]&0x1) {
      source->setValue(0x8000|((unsigned short)(source->value()))>>1);
    } else {
      source->setValue(((unsigned short)(source->value()))>>1);
    }
  } else {
    carry = source->valueByte()&0x1;
    source->setValue(source->valueByte()>>1);
  }

  int n = !!(source->value()&0x8000)<<2;
  s->data.r[2] = (s->data.r[2] & 0x04) | (!source) << 1 | carry;
  if(n) s->data.r[2] |= n;
}

void
RRA::execute(State* s) {
  if(!byte) {
    int carry = source->value()&1;
    source->setValue((source->value()&0x8000) | source->value()>>1);


    unsigned short orig = s->data.r[2];
    s->data.r[2] &= (0b11111101);
  } else {
    notimplemented();
  }
}
