#include <assert.h>
#ifndef DEST_H
#define DEST_H
#include "error.h"

// Classes representing 'Destinations' types in MSP430 - See the section
// 'MSP430 addressing modes' in https://en.wikipedia.org/wiki/TI_MSP430#MSP430_CPU

class State;

class Dest {
  // Virtual base clase.
 public:
  virtual void set(short value) { notimplemented(); }
  // Sets the value of this destination

  virtual void setByte(unsigned char value) { notimplemented(); }
  // Sets the value of this destination (byte addressing mode)

  virtual short value() { notimplemented(); return 0;}
  // Returns the value of this destination

  virtual unsigned char valueByte() { notimplemented(); return 0;}
  // Returns the value of this destination(byte addressing mode)

  virtual std::string toString() = 0;
  // Returns a string representation of this destination

  virtual bool usedExtensionWord() { return false; }
  // Whether an extension word was used to represent this destination

  virtual unsigned char size() { return usedExtensionWord() ? 2 : 0; }
  // How many extra bytes this destination took up in the assembly
};

class RegisterDest : public Dest {
  // Destination representing a register (r14)
 public:

  virtual std::string toString();

  virtual void set(short value);
  virtual void setByte(unsigned char value);
  virtual short value();
  virtual unsigned char valueByte();

  RegisterDest(unsigned short reg_) : reg(reg_) {}

  unsigned short reg;
};

class RegisterOffsetDest : public RegisterDest {
  // Destination representing the memory address at a register plus an offset (0x40(r14))
 public:
  virtual std::string toString();
  virtual bool usedExtensionWord() { return true; }
  virtual void set(short value);
  virtual void setByte(unsigned char value);
  virtual short value();
  virtual unsigned char valueByte();

 RegisterOffsetDest(unsigned short reg_, short offset_) :
   RegisterDest(reg_),
    offset(offset_) {
 }

  short offset;
};

class AbsoluteDest : public Dest {
  // Destination that is just a memory address (&0x4400)
 public:
  virtual std::string toString();
  virtual bool usedExtensionWord() { return true; }
  virtual void set(short value);
  virtual void setByte(unsigned char value);

  virtual unsigned char valueByte();

 AbsoluteDest(unsigned short address_) :
  address(address_) {
  }

  unsigned short address;
};

extern State* s;

#endif
