#pragma once

#include <iostream>

#define LOG_INFO() (std::cout << __FILE__ << ':' << __LINE__ << '(' << __FUNCTION__ << ')')
#define LOG_ERROR() (std::cerr << __FILE__ << ':' << __LINE__ << '(' << __FUNCTION__ << ')')