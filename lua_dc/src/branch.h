#pragma once

// 跨项目分支宏
// BRANCH_TOLUA     unity客户端宿主tolua
// BRANCH_SERVER    服务器端
// BRANCH_SLUA      ue4宿主端slua

// 兼容slua
#if (!defined(BRANCH_TOLUA) && (!defined(BRANCH_SERVER)))
#ifndef BRANCH_SLUA
#define BRANCH_SLUA
#endif
#endif

// 头文件
#if defined(BRANCH_TOLUA)

    #ifdef __cplusplus
    extern "C" {
    #endif

        #include "lua.h"
        #include "lualib.h"
        #include "lauxlib.h"

    #ifdef __cplusplus
    }
    #endif

#elif defined(BRANCH_SERVER)

    #ifdef __cplusplus
    extern "C" {
    #endif

        #include <third_party/lua/src/lua.h>
        #include <third_party/lua/src/lualib.h>
        #include <third_party/lua/src/lauxlib.h>

    #ifdef __cplusplus
    }
    #endif

#elif defined(BRANCH_SLUA)

    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"

    using namespace NS_SLUA;
#endif


// 导出宏
#if defined(BRANCH_TOLUA)

    #ifdef __IOS__
    #define __LINUX__
    #endif

    #ifndef __LINUX__
    #ifndef WINDOWS
    #define WINDOWS
    #endif
    #endif

    // 对外导出
    #ifdef WINDOWS
        #ifdef DLLEXPORT
            #define LDC_EXPORT __declspec(dllexport)
        #else
            #define LDC_EXPORT
        #endif
    #else // LINUX
        #define LDC_EXPORT
    #endif // WINDOWS

#elif defined(BRANCH_SERVER)

    #define LDC_EXPORT

#elif defined(BRANCH_SLUA)

    #ifdef PLATFORM_ANDROID
        #define __LINUX__
    #endif

    #ifndef __LINUX__
        #ifndef WINDOWS
            #define WINDOWS
        #endif
    #endif

    // 对外导出
    #ifdef WINDOWS
        #define LDC_EXPORT __declspec(dllexport)
    #else
        #if ((defined PLATFORM_MAC) || (defined PLATFORM_IOS))
            #define LDC_EXPORT SLUA_UNREAL_API
        #else
            #define LDC_EXPORT
        #endif
    #endif

#endif

// 在不同分支中的c和c++符号导出
#if defined(BRANCH_TOLUA)
    #define LDC_EXTERN extern "C"
#elif defined(BRANCH_SERVER)
    #define LDC_EXTERN
#elif defined(BRANCH_SLUA)
    #define LDC_EXTERN
#endif

// lua_dc函数命名空间
#if defined(BRANCH_TOLUA)
    #define LDC_FUNC_SPACE
#elif defined(BRANCH_SERVER)
     #define LDC_FUNC_SPACE _LUA_DC
#elif defined(BRANCH_SLUA)
    #define LDC_FUNC_SPACE _LUA_DC
#endif

// 宿主空间
#if defined(BRANCH_TOLUA)
    #define LDC_LUA_SPACE
#elif defined(BRANCH_SERVER)
     #define LDC_LUA_SPACE
#elif defined(BRANCH_SLUA)
    #define LDC_LUA_SPACE NS_SLUA
#endif

#if defined(BRANCH_TOLUA)
    #define LDC_LUA_EXTERN extern "C"
#elif defined(BRANCH_SERVER)
     #define LDC_LUA_EXTERN extern "C"
#elif defined(BRANCH_SLUA)
    #define LDC_LUA_EXTERN
#endif

#if defined(BRANCH_TOLUA)
    #define LDC_LUA_EXPORT LDC_EXPORT
#elif defined(BRANCH_SERVER)
     #define LDC_LUA_EXPORT LDC_EXPORT
#elif defined(BRANCH_SLUA)
    #define LDC_LUA_EXPORT LUAMOD_API
#endif


// registry表key使用
// key必须在整个宿主端保持连续,如果宿主使用的其他中间件占用了key,必须确定
// 1,2 lua内部使用
// 3-64 由tolua占用

#if defined(BRANCH_TOLUA)
    #define LDC_RIDX_START 65
#elif defined(BRANCH_SERVER)
     #define LDC_RIDX_START 3
#elif defined(BRANCH_SLUA)
    #define LDC_RIDX_START 3
#endif
