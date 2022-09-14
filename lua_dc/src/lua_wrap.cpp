#include "lua_wrap.h"

#include <sstream>

namespace _LUA_DC
{

std::string& MLuaWrap::StackDump(lua_State* L, std::string& allInfo)
{
	int stackSize = lua_gettop(L);

	for (int index = 1; index <= stackSize; index++)
	{
		int t = lua_type(L, index);
		std::string strInfo;
		switch (t)
		{
			case LUA_TSTRING:
			{
				strInfo = lua_tostring(L, index);
				break;
			}
			case LUA_TBOOLEAN:
			{
				strInfo = lua_toboolean(L, index) ? "true" : "false";
				break;
			}
			case LUA_TNUMBER:
			{
				lua_Number result = lua_tonumber(L, index);
				std::stringstream ss;
				ss << result;
				ss >> strInfo;
				break;
			}
			default:
			{
				strInfo = lua_typename(L, index);
				break;
			}
		};

		allInfo += strInfo + "\n";
	}
	return allInfo;
}

}