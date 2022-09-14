#pragma once

#include "branch.h"

namespace LDC_FUNC_SPACE
{

LDC_EXTERN  LDC_EXPORT int LuaSyntaxDataCreate(lua_State* L);         // 创建语法森林
LDC_EXTERN  LDC_EXPORT int LuaSyntaxDataRelease(lua_State* L);        // 释放语法森林

LDC_EXTERN  LDC_EXPORT int LuaInitLogFunc(lua_State* L);              // 传入输出函数
LDC_EXTERN  LDC_EXPORT int LuaInitErrFunc(lua_State* L);              // 传入输出函数

LDC_EXTERN  LDC_EXPORT int LuaInitSeria(lua_State* L);                // 初始化语法树
LDC_EXTERN  LDC_EXPORT int LuaInitRefMap(lua_State* L);            // 初始化引用类型的名字与标识映射表

LDC_EXTERN  LDC_EXPORT int LuaEncodeMessage(lua_State* L);            // 序列化网络数据对象
LDC_EXTERN  LDC_EXPORT int LuaDecodeMessage(lua_State* L);            // 反序列化网络数据

LDC_EXTERN  LDC_EXPORT int LuaEncode(lua_State* L);                   // 序列化类对象
LDC_EXTERN  LDC_EXPORT int LuaDecode(lua_State* L);                   // 反序列化类对象

LDC_EXTERN  LDC_EXPORT int LuaRawSerialize(lua_State* L);             // 供自定义序列化调用的描述式序列化
LDC_EXTERN  LDC_EXPORT int LuaRawUnSerialize(lua_State* L);           // 供自定义反序列化调用的描述式反序列化

};

// 宿主lua注入newlib入口
namespace LDC_LUA_SPACE
{

LDC_LUA_EXTERN LDC_LUA_EXPORT int luaopen_lua_dc(lua_State *L);

};
