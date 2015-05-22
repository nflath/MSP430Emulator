#include "instruction.h"
#include "state.h"
#include "util.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

// FixMe: Why is an extra letter being printed out at the end of strings?

// FixMe: Set status bits properly

// V Overflow bit. This bit is set when the result of an arithmetic operation
// overflows the signed-variable range.
// ADD(.B),ADDC(.B) Set when:
//   Positive + Positive = Negative
//   Negative + Negative = Positive,
//   otherwise reset
//   SUB(.B),SUBC(.B),CMP(.B) Set when:
//   Positive − Negative = Negative
//   Negative − Positive = Positive,
//   otherwise reset

//   N Negative bit. This bit is set when the result of a byte or word operation
//   is negative and cleared when the result is not negative.
//   Word operation: N is set to the value of bit 15 of the
//   result
//   Byte operation: N is set to the value of bit 7 of the
//   result

//   Z Zero bit. This bit is set when the result of a byte or word operation is 0
//   and cleared when the result is not 0.

//   C Carry bit. This bit is set when the result of a byte or word operation
//   produced a carry and cleared when no carry occurred

// and.b #0x-1, r5
// #status register goes from 0 to 2

// clr r15
// tst r15
// sr goes from 2 to 3

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
  unsigned char interrupt = (s->data.r[2]&0x7f00) >> 8;
  switch(interrupt) {
  case 0x00: {// putchar
    char c = (char)s->data.memory[s->data.r[1]+8];
    if((c >= 10 && c < 127)) {
      std::cout << c << std::flush;
    }
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
      str =  s->input.front();
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
    assert(!"Invalid interrupt");
    notimplemented();
  }
  unsigned short retn = s->readWord(s->data.r[1]);
  s->data.r[1] = s->data.r[1] + 2;
  s->data.r[0] = retn;
  return;
}

void
JEQ::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x2)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JL::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x4)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JC::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x1)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JN::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x1)!=0) {
    s->data.r[0] = addr+2*offset+2;
  }
}

void
JNC::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x1)==0) {
    s->data.r[0] = addr+2*offset+2;
  }
}


void
JGE::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x4)==0) {
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


// f623
// 0000 0110  23
void
JNE::execute(State* s) {
  if(s->data.r[2] == 0x0f00) {
    notimplemented();
  } else if((s->data.r[2]&0x2)==0) {
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
  }
}

void
CMP::execute(State* s) {
  if(!byte) {
    unsigned int carry = dest->value() + (~source->value()+1);
    short result =  dest->value() - source->value();
    //std::cout << "Comparing: " << dest->value() << " " << source->value() << " " << (~carry >> 16)&0x1 << std::endl;
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
  if(!byte) {
    short result = (dest->value())&(source->value());
    dest->set(result);
    s->data.r[2] = (result < 0) << 2 | (result == 0) << 1 | 0;
  } else {
    // FixMe: Is this correct?
    dest->set((dest->value() & 0xff)&(source->value() & 0xff));
  }
}

void
ADD::execute(State* s) {
  if(!byte) {
    unsigned int dv = (0x0000ffff & (unsigned int)dest->value());
    unsigned int sv = (0x0000ffff & (unsigned int)source->value());
    unsigned int result =  dv + sv;

    dest->set(dest->value()+source->value());



    s->data.r[2] =
      ((dest->value()&0x8000)>>13) |
      ((dest->value() == 0) << 1) |
      ((result & 0x10000) >> 16);

  } else {
    dest->setByte((dest->valueByte() + (source->valueByte())));
    s->data.r[2] = 0x0f00;
  }
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
    // FixMe: Add carry bit calculations
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
};

BcdResult
addBcd(unsigned int val1, unsigned int val2) {
  unsigned short carry = 0; // FixMe:???
  unsigned short result = 0;
  for(int position = 0; position < 4; position++) {
    unsigned char nibble =
      carry +
      (((val1 & (0xf << position * 4))) >> (position * 4)) +
      (((val2 & (0xf << position * 4))) >> (position * 4));
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
  return bcd;
}

void
DADD::execute(State* s) {
  if(!byte) {
    // FixMe: Add carry bit calculations

    BcdResult bcd = addBcd(dest->value(),source->value());

    dest->set(bcd.result);

    s->data.r[2] =
      ((bcd.result < 0) << 2) |
      ((bcd.result == 0) << 1) |
      bcd.carry;
    //FixMe: not complete


  } else {
    notimplemented();
    unsigned short result = convertBcdToHex(dest->valueByte()) +
      convertBcdToHex(source->valueByte());

    dest->setByte(convertHexToBcd(result));

    //FixMe: What else does DADD add?

  }
}

void
SUB::execute(State* s) {
  if(!byte) {
    int carry = (source->value() > dest->value())?0:1;
    dest->set((dest->value() - (source->value())));

    s->data.r[2] =
      ((dest->value() < 0) << 2) |
       ((dest->value() == 0) << 1) |
       carry; // FixMe: is this right? (ANSWER: NO)



  } else {
    assert(!"Not implemented");
  }
}

void
BIC::execute(State* s) {
  if(!byte) {
    unsigned short result = (dest->value())&~(source->value());
    dest->set(result);
    s->data.r[2] = (result>>15)<<2 |
      (result==0)<<1 |
      0;
  } else {
    assert(!"Not implemented");
  }
}

void
BIT::execute(State* s) {
  if(!byte) {
    unsigned short result = (dest->value())&(source->value());
    s->data.r[2] = (result>>15)<<2 |
      (result==0)<<1 |
      0;
  } else {
    notimplemented();
  }
}

void
BIS::execute(State* s) {
  if(!byte) {
    dest->set((dest->value())|(source->value()));
  } else {
    assert(!"Not implemented");
  }
}

void
XOR::execute(State* s) {
  if(!byte) {
    dest->set((dest->value())^(source->value()));
  } else {
    dest->setByte((dest->valueByte())^(source->valueByte()));
  }
}

void
RRC::execute(State* s) {
  if(!byte) {
    // FixMe: Is this correct?
    int carry = source->value()&0x1;
    if(s->data.r[2]&0x1) {
      source->setValue(0x8000|((unsigned short)(source->value()))>>1);
    } else {
      source->setValue(((unsigned short)(source->value()))>>1);
    }
    s->data.r[2] = carry;

  } else {

    source->setValue(source->valueByte()>>1);
  }
}

void
RRA::execute(State* s) {
  if(!byte) {
    // FixMe: Is this correct
    source->setValue(source->value()>>1);
  } else {
    notimplemented();
  }
}
