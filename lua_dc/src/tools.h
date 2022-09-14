#pragma once

#include <vector>
#include <string>
#include <string.h>

#include "seria.h"
#include "def.h"
#include "log.h"

namespace _LUA_DC
{

class MTools
{
public:
	static char* DelChar(char* pstr, char del)
	{
		if (NULL == pstr)
		{
			return NULL;
		}

		const int len = (int)strlen(pstr);
		if (len == 0)
		{
			return NULL;
		}

		int index = 0;
		for (int i = 0; i < len; ++i)
		{
			if (del == pstr[i])
			{
				continue;
			}
			pstr[index++] = pstr[i];
		}

		pstr[index++] = '\0';
		return pstr;
	}

	static char* ReplaceChar(char* pstr, char pre, char neo)
	{
		if (NULL == pstr)
		{
			return NULL;
		}

		const int len = (int)strlen(pstr);
		if (len == 0)
		{
			return NULL;
		}

		for (int i = 0; i < len; ++i)
		{
			if (pre == pstr[i])
			{
				pstr[i] = neo;
			}
		}

		return pstr;
	}
    
	static std::vector<std::string>& SplitStr(char* pstr, const char* sp, std::vector<std::string>& vec)
	{
		if ((NULL == pstr) || (NULL == sp))
		{
			return vec;
		}

		char *p = strtok(pstr, sp);
		while (p)
		{
			vec.push_back(p);
			p = strtok(NULL, sp);
		}
		return vec;
	}

	static bool SplitStr2Tb(std::string& str, std::vector<std::string>& str_tb)
	{
		int word_start = -1;
		std::string word;
		for (int i = 0; i < (int)str.length(); ++i)
		{
			std::string sub2 = str.substr(i, 1);
			char s_c = sub2[0];
			if ((isalpha(s_c) != 0) || (isdigit(s_c) != 0) || (s_c == '_') || (s_c == '&'))
			{
				if (-1 == word_start)
				{
					word_start = i;
				}
			}
			else
			{
				if (-1 != word_start)
				{
					word = str.substr(word_start, i - word_start);
					word_start = -1;
					str_tb.push_back(word);
				}

				if (! ((s_c == ' ' || s_c == '\t')) )
				{
					str_tb.push_back(sub2);
				}
			}
		}

		if (-1 != word_start)
		{
			word = str.substr(word_start, -1);
			word_start = -1;
			str_tb.push_back(word);
		}

		return true;
	}

	static bool IsTokenChar(char c)
	{
		const static std::unordered_map<char, bool> spec = { {'<',true},{'>',true},{',',true} };
		return (spec.end() != spec.find(c));
	}

	static bool IsAvailString(const std::string& str)
	{
		size_t len = str.size();
		for (size_t i = 0; i < len; ++i)
		{
			const char c = str[i];
			if ((!(isalpha(c) != 0)) && (!IsTokenChar(c)) && (!(isdigit(c) != 0)) && (c!='_') && (c!='&'))
			{
				LDC_LOG_ERR("invalid char, i="<<i);
				return false;
			}

			if ( (i>0) && (str[i-1] == c) && (c=='&') ) // 连续的&&非法
			{
				LDC_LOG_ERR("invalid double &, i="<<i);
				return false;
			}
		}

		return true;
	}
};

}