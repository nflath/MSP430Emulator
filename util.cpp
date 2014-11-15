#include "util.h"

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
