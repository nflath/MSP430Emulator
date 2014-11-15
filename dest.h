#include <assert.h>
#ifndef DEST_H
#define DEST_H
#include "error.h"

class State;

class Dest {
 public:
  virtual void set(short value) { notimplemented(); }
  virtual void setByte(unsigned char value) { notimplemented(); }
  virtual short value() { notimplemented(); return 0;}
  virtual unsigned char valueByte() { notimplemented(); return 0;}
  virtual std::string toString() = 0;
  virtual unsigned char size() { return usedExtensionWord() ? 2 : 0; }
  virtual bool usedExtensionWord() { return false; }
};

class RegisterDest : public Dest {
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
