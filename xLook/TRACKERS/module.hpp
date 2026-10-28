#pragma once
#include <windows.h>
void trimRight(char * text);
int lstrcmpn(char * a, char * b, int max);
bool search_with_table(const BYTE * data, const BYTE * pattern, int size);
