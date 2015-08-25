#include "util.h"

int
strToR(const std::string str) {
  if(str == "pc") {
    return 0;
  } else if(str == "sr") {
    return 2;
  } else if(str == "sp") {
    return 1;
  }
  if(str[0] == 'r') {
    unsigned long value = strtoul(str.substr(1).c_str(),NULL,10);
    return value;
  }

  return -1;
}

std::string
readString() {
  std::string retn;
  std::cin >> retn;
  return retn;
}

std::string
readLine() {
  std::string retn;
  getline(std::cin, retn);
  return retn;
}
