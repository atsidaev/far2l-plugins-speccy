#ifndef WIDESTRING_HPP
#define WIDESTRING_HPP
#include <string>
#include <fstream>

std::wstring _W(char* s);
std::string _N(wchar_t* s);

const wchar_t* _WW(const char*);
const char* _NN(const wchar_t*);

#endif
