#include "seria.h"
#include "def.h"
#include "seria_helper.h"
#include "seria_build.h"
#include "seria_encode.h"
#include "seria_decode.h"
#include "log.h"

#include <assert.h>

using namespace _LUA_DC;

namespace LDC_FUNC_SPACE
{

// 生命周期的初始化阶段,尽快的宕掉以发现问题

int LuaSyntaxDataCreate(lua_State* L)
{
    // registry的key占位
    for (int i=LDC_RIDX_SYNTAX; i<LDC_RIDX_END; ++i)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, i);
        if (!lua_isnil(L, -1))
        {
            LDC_LOG_ERR("luadc create fail, registry key not nil "<<i<<" "<<lua_type(L, -1));
            assert(0);
        }

        lua_pop(L, 1);

        lua_pushinteger(L, i); // 占位
		lua_rawseti(L, LUA_REGISTRYINDEX, i);
    }

    MLog::SetLuaState(L);
	SSyntax* pSyntax = new SSyntax();
	lua_pushlightuserdata(L, (void*)pSyntax);
    lua_rawseti(L, LUA_REGISTRYINDEX, LDC_RIDX_SYNTAX);
    return 0;
}

int LuaSyntaxDataRelease(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, LDC_RIDX_SYNTAX);
	if (!lua_islightuserdata(L, -1))
	{
		LDC_LOG_ERR("warn: syntax has not create!");
		lua_pop(L, 1);
		return 0;
	}

    SSyntax* pSyntax = static_cast<SSyntax*>(lua_touserdata(L,-1));
    delete pSyntax;
    pSyntax = nullptr;

    for (int i=LDC_RIDX_SYNTAX; i<LDC_RIDX_END; ++i)
    {
        lua_pushnil(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, i); // 标记为nil
    }

    return 0;
}

int LuaInitSeria(lua_State* L)
{
    LDC_LOG_DEBUG(__FUNCTION__);
    lua_rawgeti(L, LUA_REGISTRYINDEX, LDC_RIDX_SYNTAX);
    assert(lua_islightuserdata(L, -1));
    SSyntax* pSyntax = static_cast<SSyntax*>(lua_touserdata(L,-1));
    lua_pop(L, 1);
    MSeriaBuild::BuildMessage(L, *pSyntax);
    return 2; 
}

int LuaInitRefMap(lua_State* L)
{
    LDC_LOG_DEBUG(__FUNCTION__);
    lua_rawgeti(L, LUA_REGISTRYINDEX, LDC_RIDX_SYNTAX);
    assert(lua_islightuserdata(L, -1));
    SSyntax* pSyntax = static_cast<SSyntax*>(lua_touserdata(L,-1));
    lua_pop(L, 1);
    MSeriaBuild::BuildRefMap(L, *pSyntax);
    return 2; 
}

// 以下函数为lua初始化阶段

int LuaInitLogFunc(lua_State* L)
{
    if (!MLog::InitLogFunc(L))
    {
        lua_pushnil(L);
        lua_pushfstring(L, "MLog::InitLogFunc fail");
        return 2;
    }

    lua_pushboolean(L, 1);
    lua_pushfstring(L, "MLog::InitLogFunc success");
    return 2;
}

int LuaInitErrFunc(lua_State* L)
{
    if (!MSeriaHelper::InitErrFunc(L))
    {
        lua_pushnil(L);
        lua_pushfstring(L, "MLog::InitErrFunc fail");
        return 2;
    }

    lua_pushboolean(L, 1);
    lua_pushfstring(L, "MLog::InitErrFunc success");
    return 2;
}

// 注意,以下接口为运行阶段

int LuaEncodeMessage(lua_State* L)
{
    SSyntax* pSyntax{nullptr};
    DEF_OBTAIN_SSYNTAX(L, LDC_RIDX_SYNTAX, pSyntax);
    MSeriaEncode::EncodeNetMessage(L, *pSyntax);
    return 2;
}

int LuaEncode(lua_State* L)
{
    SSyntax* pSyntax{nullptr};
    DEF_OBTAIN_SSYNTAX(L, LDC_RIDX_SYNTAX, pSyntax);
    MSeriaEncode::EncodeMemoryObject(L, *pSyntax);
    return 2;
}

int LuaDecodeMessage(lua_State* L)
{
    SSyntax* pSyntax{nullptr};
    DEF_OBTAIN_SSYNTAX(L, LDC_RIDX_SYNTAX, pSyntax);
    MSeriaDecode::DecodeNetMessage(L, *pSyntax);
    return 2;
}

int LuaDecode(lua_State* L)
{
    SSyntax* pSyntax{nullptr};
    DEF_OBTAIN_SSYNTAX(L, LDC_RIDX_SYNTAX, pSyntax);
    MSeriaDecode::DecodeMemoryObject(L, *pSyntax);
    return 2;
}

int LuaRawSerialize(lua_State* L)
{
    SSyntax* pSyntax{nullptr};
    DEF_OBTAIN_SSYNTAX(L, LDC_RIDX_SYNTAX, pSyntax);
    MSeriaEncode::RawSerialize(L, *pSyntax);
    return 2;
}

int LuaRawUnSerialize(lua_State* L)
{
    SSyntax* pSyntax{nullptr};
    DEF_OBTAIN_SSYNTAX(L, LDC_RIDX_SYNTAX, pSyntax);
    MSeriaDecode::RawUnSerialize(L, *pSyntax);
    return 2;
}

};

namespace LDC_LUA_SPACE
{
 
int luaopen_lua_dc(lua_State *L)
{
    luaL_Reg libs[] = {
        { "Create",             LDC_FUNC_SPACE::LuaSyntaxDataCreate },
        { "Release",            LDC_FUNC_SPACE::LuaSyntaxDataRelease },        

        { "InitLogFunc",	    LDC_FUNC_SPACE::LuaInitLogFunc },
        { "InitErrFunc",	    LDC_FUNC_SPACE::LuaInitErrFunc },

        { "InitSeria",			LDC_FUNC_SPACE::LuaInitSeria },			
        { "InitRefMap",	        LDC_FUNC_SPACE::LuaInitRefMap },			

        { "ToStream",			LDC_FUNC_SPACE::LuaEncodeMessage },
        { "FromStream",			LDC_FUNC_SPACE::LuaDecodeMessage },

        { "SaveStream",			LDC_FUNC_SPACE::LuaEncode },		    
        { "LoadStream",			LDC_FUNC_SPACE::LuaDecode },		    

        { "RawSerialize",		LDC_FUNC_SPACE::LuaRawSerialize },		    
        { "RawUnSerialize",		LDC_FUNC_SPACE::LuaRawUnSerialize },

        { NULL, NULL }
    };
    
    luaL_newlib(L, libs);
    return 1;
}

};

