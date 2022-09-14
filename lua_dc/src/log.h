#pragma once

#include "seria.h"
#include "def.h"

#include <fstream>
#include <sstream>
#include <stdio.h>
#include <time.h>

#ifdef BRANCH_SLUA
#pragma warning(disable:4457)
#pragma warning(disable:4996)
#endif

namespace _LUA_DC 
{

class MLog
{
public:
    static bool InitLogFunc(lua_State* L)
    {
        if (!lua_isfunction(L, -1))
        {
            printf("fatal: InitLogFunc L->top not func\n");
            return false;
        }

        lua_rawseti(L, LUA_REGISTRYINDEX, LDC_RIDX_FUNC_LOG);
        return true;
    }

    // 成功,栈顶为func;失败时堆栈依然要平衡
    static bool GetLogFunc()
    {
        if (!m_lua_state)
        {
            return false;
        }

        lua_State* L = m_lua_state;
        lua_rawgeti(L, LUA_REGISTRYINDEX, LDC_RIDX_FUNC_LOG);
        if (!lua_isfunction(L, -1))
        {
            printf("fatal: GetLogFunc L->top not func\n");
            lua_pop(L, 1);
            return false;
        }

        return true;
    }

    static void AppendFile(const char* str)
    {
        if (!m_file)
        {
            if ((m_file = fopen(LOG_FILE_SELF, "a")) == NULL)
            {
                printf("fatal: open log file fail! [%s]\n", LOG_FILE_SELF);
                return;
            }
        }

        if (EOF == fputs(str, m_file))
        {
            printf("fatal: write log file fail! [%s] [%s]\n", LOG_FILE_SELF, str);
            return;
        }
    }

    static char* TimeStr()
    {
        time_t t = time(NULL);
        tm* local = localtime(&t);
        static char ti[56] = {0};
        memset(ti, 0x00, 56);
        sprintf(ti, "%d-%d-%d.%d_%d_%d",
            local->tm_year+1900, local->tm_mon+1, local->tm_mday,
            local->tm_hour, local->tm_min, local->tm_sec);

        return ti;
    }

    static void SetLuaState(lua_State* L) 
    {
        m_lua_state = L; 
    };

    static lua_State* GetLuaState() {return m_lua_state;};

private:
    static FILE* m_file;
    static lua_State* m_lua_state;
};

#define LUADC_LOG(dStream) \
do { \
    std::stringstream dSs; \
    if (MLog::GetLogFunc()) { \
        dSs << dStream; \
        const std::string& dSstr = dSs.str(); \
        lua_State* dL = MLog::GetLuaState(); \
        lua_pushstring(dL, dSstr.c_str()); \
        lua_call(dL, 1, 0); \
    } else { \
        dSs << MLog::TimeStr() << ':'; \
        dSs << dStream << '\n'; \
        const std::string& dSstr = dSs.str(); \
        MLog::AppendFile(dSstr.c_str()); \
    } \
}  while (0);

#define LDC_LOCATION __FILE__<<":"<<__LINE__

#define LDC_LOG_ERR(stream)    LUADC_LOG("fatal "<<LDC_LOCATION<<":"<<stream)
#define LDC_LOG_WARN(stream)   LUADC_LOG("warn "<<LDC_LOCATION<<":"<<stream)

#ifdef LUADC_DEBUG
#define LDC_LOG_INFO(stream)   LUADC_LOG("info "<<LDC_LOCATION<<":"#stream<<","<<stream)
#define LDC_LOG_DEBUG(stream)  LUADC_LOG("debug "<<LDC_LOCATION<<":"<<#stream<<","<<stream)
#else //LUADC_DEBUG
#define LDC_LOG_INFO(stream)
#define LDC_LOG_DEBUG(stream)
#endif //LUADC_DEBUG

}