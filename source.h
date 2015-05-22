#ifndef SOURCE_H
#define SOURCE_H

#include <string>
#include <assert.h>
#include "error.h"

class State;

class Source {
 public:
  unsigned short* memLocationOfSource;

  virtual std::string toString() = 0;
  // FixMe: This will be necessary

  virtual short value(bool byte=false) = 0;
  unsigned char valueByte() {
    return value(true)&0xff;
  }
  void setValue(unsigned short val) {
    if(!memLocationOfSource) {
      notimplemented();
    }
    *memLocationOfSource = val;
  }

  bool usedExtensionWord;
  virtual unsigned char size() { return usedExtensionWord ? 2 : 0; }

};

class Absolute : public Source {
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
 public:

  virtual std::string toString();
  virtual short value(bool byte=false);

 RegisterIndirectAutoincrementSource(unsigned short reg_, unsigned char* memLocationOfSource_) :
  RegisterIndirectSource(reg_, memLocationOfSource_) {
    usedExtensionWord = false;
  }
};

#endif
