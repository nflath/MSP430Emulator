#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include "state.h"

// Few simple utility functions used in mutliple places

int strToR(const std::string str);
// Convert a string value into the integer representing the register

std::string readString();
// Read a string from user

std::string readLine();
// Read a line from user

#endif
