#ifndef SOURCE_H
#define SOURCE_H

#include <string>
#include <assert.h>
#include "error.h"

class State;

class Source {
 public:
  virtual std::string toString() = 0;
  virtual short value() = 0;
  virtual void setValue(unsigned short val) { notimplemented(); }
  virtual unsigned char valueByte() = 0;
  virtual bool usedExtensionWord() {
    return false;
  }
  virtual unsigned char size() { return usedExtensionWord() ? 2 : 0; }
};

class Absolute : public Source {
 public:
  unsigned short address;

  virtual std::string toString();
  virtual bool usedExtensionWord() {
    return true;
  }

  virtual short value();
  virtual unsigned char valueByte();

 Absolute(short address_):
  address(address_) {
  }
};

class Constant : public Source {
 public:
  unsigned short val;
  bool extWordUsed;

  virtual bool usedExtensionWord() {
    return extWordUsed;
  }

  virtual std::string toString();

  virtual short value() {
    return val;
  }
  virtual unsigned char valueByte();

 Constant(short value_,bool extWordUsed_):
  val(value_),
    extWordUsed(extWordUsed_) {

  }
};

class RegisterSource : public Source {
 public:
  unsigned short reg;

  virtual short value();
  virtual unsigned char valueByte();
  virtual void setValue(unsigned short val);
  virtual std::string toString();

 RegisterSource(unsigned short reg_) :
  reg(reg_) {
  }

};

class RegisterIndirectSource : public RegisterSource {
 public:

  virtual std::string toString();
  virtual short value();
  virtual unsigned char valueByte();

 RegisterIndirectSource(unsigned short reg_) :
  RegisterSource(reg_) {
  }
};

class RegisterIndexedSource : public RegisterSource {
 public:
  short index;

  virtual std::string toString();
  virtual short value();
  virtual unsigned char valueByte();

  virtual bool usedExtensionWord() {
    return true;
  }

 RegisterIndexedSource(unsigned short reg_, short index_) :
  RegisterSource(reg_),
    index(index_) {
  }
};

class RegisterIndirectAutoincrementSource : public RegisterIndirectSource {
 public:

  virtual std::string toString();
  virtual short value();
  virtual unsigned char valueByte();

 RegisterIndirectAutoincrementSource(unsigned short reg_) :
  RegisterIndirectSource(reg_) {
  }
};

#endif
