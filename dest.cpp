#include <iostream>
#include <sstream>
#include <iomanip>

#include "dest.h"
#include "state.h"

std::string
RegisterDest::toString() {
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

void
RegisterDest::set(short value) {
  s->data.r[reg] = value;
}

void
RegisterDest::setByte(unsigned char value) {
  s->data.r[reg] = value;
}

short
RegisterDest::value() {
  return s->data.r[reg];
}

unsigned char
RegisterDest::valueByte() {
  return s->data.r[reg];
}

std::string
RegisterOffsetDest::toString() {
  std::stringstream ss;
  ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << offset << "("  << RegisterDest::toString() << ")" ;
  return ss.str();
}

void
RegisterOffsetDest::set(short value) {
  s->writeByte(s->data.r[reg]+offset+1, (value&0xff00) >> 8);
  s->writeByte(s->data.r[reg]+offset, (value&0x00ff));
}

void
RegisterOffsetDest::setByte(unsigned char value) {
  s->writeByte(s->data.r[reg]+offset, value);
}

unsigned char
RegisterOffsetDest::valueByte() {
  return s->data.memory[s->data.r[reg]+offset];
}

short
RegisterOffsetDest::value() {
  return s->readWord(s->data.r[reg]+offset);
}

void
AbsoluteDest::set(short value) {
  s->writeByte(address+1, (value&0xff00) >> 8);
  s->writeByte(address, (value&0x00ff));
}

void
AbsoluteDest::setByte(unsigned char value) {
  s->writeByte(address,value);
}

unsigned char
AbsoluteDest::valueByte() {
  return s->data.memory[address];
}

std::string
AbsoluteDest::toString() {
   std::stringstream ss;
   ss << "&0x" << std::hex << std::setw(4) << std::setfill('0') << address;
   return ss.str();
}
