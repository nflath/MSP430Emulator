#include <iostream>
#include <sstream>
#include <iomanip>

#include "source.h"
#include "state.h"

std::string
Absolute::toString() {
  std::stringstream ss;
  ss << "&0x" << std::hex << std::setw(4) << std::setfill('0') << address;
  return ss.str();
}

std::string
Constant::toString() {
  std::stringstream ss;
  ss << "#0x" << std::hex << std::setw(4) << std::setfill('0') << val;
  return ss.str();
}

short
Absolute::value() {
  return s->readWord(address);
}

unsigned char
Constant::valueByte() {
  return val;
}

unsigned char
Absolute::valueByte() {
  return s->readByte(address);
}

short
RegisterSource::value() {
  return s->data.r[reg];
}

void
RegisterSource::setValue(unsigned short val) {
  s->data.r[reg] = val;
}

unsigned char
RegisterSource::valueByte() {
  return s->data.r[reg] & 0x00ff;
}

short
RegisterIndirectSource::value() {
  return s->readWord(s->data.r[reg]);
}

unsigned char
RegisterIndirectSource::valueByte() {
  return s->data.memory[s->data.r[reg]];
}

short
RegisterIndirectAutoincrementSource::value() {
  unsigned short retn = s->readWord(s->data.r[reg]);
  s->data.r[reg]+=2; // FixMe; is it always 2?

  return retn;
}

unsigned char
RegisterIndirectAutoincrementSource::valueByte() {
  unsigned short retn = s->readByte(s->data.r[reg]);
  s->data.r[reg]+=1; // FixMe; is it always 2?

  return 0;
}

short
RegisterIndexedSource::value() {
  return s->readWord(s->data.r[reg]+index);
}

unsigned char
RegisterIndexedSource::valueByte() {
  return s->readByte(s->data.r[reg]+index);
}

std::string
RegisterSource::toString() {
  switch(reg) {
  case 0:
    return "PC";
  case 1:
    return "sp";
  case 2:
    return "sr";
  case 3:
    return "cg";
  default:
    std::stringstream ss;
    ss << "r" << reg;
    return ss.str();
  }
}

std::string
RegisterIndexedSource::toString() {
  std::stringstream ss;
  ss << std::hex << "0x" << std::setw(4) << index << "(" << RegisterSource::toString() << ")";
  return ss.str();
}

std::string
RegisterIndirectSource::toString() {
  return "@" + this->RegisterSource::toString();
}

std::string
RegisterIndirectAutoincrementSource::toString() {
  return "@" + this->RegisterSource::toString() + "+";
}
