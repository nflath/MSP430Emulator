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
Absolute::value(bool byte) {
  return s->readWord(address,byte);
}

short
RegisterSource::value(bool byte) {
  return s->data.r[reg];
}

short
RegisterIndirectSource::value(bool byte) {
  return s->readWord(s->data.r[reg],byte);
}


short
RegisterIndirectAutoincrementSource::value(bool byte) {
  unsigned short retn = s->readWord(s->data.r[reg],byte);
  s->data.r[reg]+=byte?1:2;


  return retn;
}

short
RegisterIndexedSource::value(bool byte) {
  return s->readWord(s->data.r[reg]+index,byte);
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
