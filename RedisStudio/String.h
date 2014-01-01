#pragma once

#include <string>
#include <vector>

class String
{
public:
	typedef std::vector<std::string> TSeqStr;
public:
	enum Options
	{
		TOK_IGNORE_EMPTY = 1, /// ignore empty tokens
		TOK_TRIM         = 2  /// remove leading and trailing whitespace from tokens
	};

	static bool IsSpace(const char& ch);

	static void Split(const std::string& str, const std::string& separators, TSeqStr& tokens, int options=0);
};
