#pragma once

#include "seria.h"
#include "def.h"
#include "tools.h"
#include "lua_wrap.h"
#include "seria_helper.h"

namespace _LUA_DC
{

class MSeriaBuild
{
public:
    static bool BuildMessage(lua_State* L, SSyntax& syntax);   // 构建语法树
    static bool BuildRefMap(lua_State* L, SSyntax& syntax); // 构建引用数据的映射表


private:
    static bool BuildField(lua_State* L, SMessage& message);   // 构建属性
    static bool BuildFather(lua_State* L, SSyntax& syntax);    // 构建基类
    static bool BuildCheckFather(SSyntax& syntax);             // 构建的语法森林完整性校验,基类引用

    static bool BuildMessage(lua_State* L, SSyntax& syntax, const int off);

};

}