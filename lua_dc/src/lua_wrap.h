#pragma once

#include "seria.h"
#include "def.h"
#include "buffer/NetBuffer.h"
#include "log.h"

namespace _LUA_DC
{

class MLuaWrap
{
public:
	// 打印堆栈错误信息
	static std::string& StackDump(lua_State* L, std::string& allInfo);
};

#define LDC_TYPE_CHECK(dL,dIdx,dType)			\
do {											\
    int dLtype = lua_type(dL, dIdx);			\
    if (dType != dLtype) {						\
        LDC_LOG_ERR("dType err!"				\
			<< ",dType="						\
			<< lua_typename(dL, dType)			\
			<< ",dLtype="						\
			<< lua_typename(dL, dLtype) 		\
			<< ",dIdx="<<dIdx);					\
        return false;							\
    }											\
} while (0);


#define LUA_STACK_KEY(L, key, off)	\
	lua_pushstring(L,key);	\
	lua_gettable(L, off);

}