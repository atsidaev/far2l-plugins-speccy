#include "widestring.hpp"
#include <windows.h>
#include <codecvt>
#include <locale>

std::wstring _W(char* s)
{
  typedef std::codecvt_utf8<wchar_t> convert_type;
  std::wstring_convert<convert_type, wchar_t> converter;
  std::wstring result = converter.from_bytes(s);

  return result;
}

std::string _N(wchar_t* s)
{
	int size = wcslen(s);
	std::wstring wc(s);

	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	std::string converted_str = converter.to_bytes(wc);
	return converted_str;
}

