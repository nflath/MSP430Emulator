#ifndef SOURCE_H
#define SOURCE_H

#include <string>
#include <assert.h>
#include "error.h"

// Classes that represent 'sources' in MSP430 assembly

class State;

class Source {
 public:
  unsigned short* memLocationOfSource;
  // actual memory location of this source.

  virtual std::string toString() = 0;
  // String representing this source

  virtual short value(bool byte=false) = 0;
  unsigned char valueByte() {
    return value(true)&0xff;
  }
  // Return the vlaue of this source(in word or byte length).

  virtual void setValue(unsigned short val) {
    // Set the value of the source (used for single-operand instructions)
    if(!memLocationOfSource) {
      notimplemented();
    } else {
      *memLocationOfSource = val;
    }
  }

  bool usedExtensionWord;
  // Whether the representation of this source used an extension word.

  virtual unsigned char size() { return usedExtensionWord ? 2 : 0; }
  // The number of bytes the representation of this source takes

};

class Absolute : public Source {
  // Absolute memory address (&0x4400)
 public:
  unsigned short address;

  virtual std::string toString();
  virtual short value(bool byte=false);

 Absolute(short address_):
   address(address_) {
    usedExtensionWord = true;
  }
};

class Constant : public Source {
  // Constrant (#0x1)
 public:
  unsigned short val;
  bool extWordUsed;

  virtual std::string toString();

  virtual short value(bool byte=false) {
    return val;
  }

 Constant(short value_,bool extWordUsed_):
  val(value_),
    extWordUsed(extWordUsed_) {
     usedExtensionWord = extWordUsed;
  }
};

class RegisterSource : public Source {
  // Register (r15)
 public:
  unsigned short reg;

  virtual short value(bool byte=false);
  virtual std::string toString();

 RegisterSource(unsigned short reg_, unsigned char* memLocationOfSource_) :
  reg(reg_) {
    memLocationOfSource = (unsigned short*)memLocationOfSource_;
    usedExtensionWord = false;
  }

};

class RegisterIndirectSource : public RegisterSource {
  // Register indirect (@r15)
 public:

  virtual std::string toString();
  virtual short value(bool byte);

 RegisterIndirectSource(unsigned short reg_, unsigned char* memLocationOfSource_) :
  RegisterSource(reg_,0) {
    memLocationOfSource = (unsigned short*)memLocationOfSource_;
    usedExtensionWord = false;
  }
};

class RegisterIndexedSource : public RegisterSource {
  // Memory location offset from register (0x0100(r15))
 public:
  short index;

  virtual std::string toString();
  virtual short value(bool byte=false);

 RegisterIndexedSource(unsigned short reg_, short index_) :
  RegisterSource(reg_,0),
    index(index_) {
      usedExtensionWord = true;
  }
};

class RegisterIndirectAutoincrementSource : public RegisterIndirectSource {
  // Register indirect source, and then increment the register (@r15+)
 public:

  virtual std::string toString();
  virtual short value(bool byte=false);

 RegisterIndirectAutoincrementSource(unsigned short reg_, unsigned char* memLocationOfSource_) :
  RegisterIndirectSource(reg_, memLocationOfSource_) {
   usedExtensionWord = false;
  }
};

#endif
