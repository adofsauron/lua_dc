#pragma once

#include "seria.h"
#include "def.h"
#include "tools.h"
#include "log.h"

#include <assert.h>

namespace _LUA_DC
{

class MSeriaHelper
{

private:
    static std::unordered_map<std::string, EValueType> m_types;

public:

    EValueType static GetValueType(const std::string& ft)
    {
        auto iter = m_types.find(ft);

        if (iter != m_types.end())
        {
            return iter->second;
        }

        if (ft.empty())
        {
            LDC_LOG_ERR("ft empty!");
            return EValueType::NotSupported;
        }

        if (ft == "short")
        {
            m_types[ft] = EValueType::Int32;
            return EValueType::Int32;
        }
        else if (ft == "ushort")
        {
            m_types[ft] = EValueType::Int32;
            return EValueType::Int32;
        }
        else if (ft == "int")
        {
            m_types[ft] = EValueType::Int32;
            return EValueType::Int32;
        }
        else if (ft == "uint")
        {
            m_types[ft] = EValueType::Int32;
            return EValueType::Int32;
        }
        else if (ft == "int64")
        {
            m_types[ft] = EValueType::Int64;
            return EValueType::Int64;
        }
        else if (ft == "uint64")
        {
            m_types[ft] = EValueType::Int64;
            return EValueType::Int64;
        }
        else if (ft == "float")
        {
            m_types[ft] = EValueType::Float32;
            return EValueType::Float32;
        }
        else if (ft == "double")
        {
            m_types[ft] = EValueType::Float64;
            return EValueType::Float64;
        }
        else if (ft == "string")
        {
            m_types[ft] = EValueType::String;
            return EValueType::String;
        }
        else if (ft == "bool")
        {
            m_types[ft] = EValueType::Int32;
            return EValueType::Int32;
        }
        else if (ft == "buffer")
        {
            m_types[ft] = EValueType::ENetBuffer;
            return EValueType::ENetBuffer;
        }
        else if (ft == "time")
        {
            m_types[ft] = EValueType::Time64;
            return EValueType::Time64;
        }
        else if (ft == "list")
        {
            m_types[ft] = EValueType::List;
            return EValueType::List;
        }
        else if (ft == "map")
        {
            m_types[ft] = EValueType::Map;
            return EValueType::Map;
        }
        else if (ft == "byte")
        {
            m_types[ft] = EValueType::Int32;
            return EValueType::Int32;
        }
        else if (ft == "any")
        {
            m_types[ft] = EValueType::Any;
            return EValueType::Any;
        }
        else if (ft.back() == '&')
        {
            constexpr size_t objStrLen = 6; // Object有6个字节, 直接写出来
            if ( (ft.length() == (size_t) (objStrLen+1))
                && (0 == memcmp(ft.c_str(), STR_OBJECT, objStrLen)) ) // Object&视作any
            {
                m_types[ft] = EValueType::Any;
                return EValueType::Any;
            }

            m_types[ft] = EValueType::Reference;
            return EValueType::Reference;
        }
        else
        {
            m_types[ft] = EValueType::Compound;
            return EValueType::Compound;
        }
    }

    EWireType static GetWireType(const EValueType& vt)
    {
        switch (vt)
        {
            case (EValueType::Int32):
            {
    #ifdef USE_VAR_INT32
                return EWireType::EVarInt;
    #else
                return EWireType::Data32;
    #endif //USE_VAR_INT32
            }
            break;

            case (EValueType::Int64):
            case (EValueType::Time64):
            {
    #ifdef USE_VAR_INT64
                return EWireType::EVarInt;
    #else
                return EWireType::Data64;
    #endif//USE_VAR_INT64            
            }
            break;

            case (EValueType::Float32):
            {
                return EWireType::Data32;
            }
            break;

            case (EValueType::Float64):
            {
                return EWireType::Data64;
            }
            break;

            case (EValueType::String):
            {
                return EWireType::LengthDelVarUInt;
            }
            break;

            case (EValueType::ENetBuffer):
            {
                return EWireType::LengthDelVarUInt;
            }
            break;

            default:
            {
                return EWireType::LengthDelimited;
            }
        }
    }

    // 根据类名创建类的实例
    bool static NewObject(lua_State* L, const char* className)
    {
        lua_getglobal(L, className); // Stack: _G, class table
        if (!lua_istable(L, -1))
        {
            LDC_LOG_ERR("top no table!"<<className);
            lua_pop(L, 1);
            return false;
        }
        lua_getfield(L, -1, "New");
        if (!lua_isfunction(L, -1)) // Stack: _G, class table, New function
        {
            LDC_LOG_ERR("top no function!"<<className);
            lua_pop(L, 1);
            return false;
        }

        lua_pushvalue(L, -2); // Stack: _G, class table, New function, class table
        lua_call(L, 1, 1); // Stack: _G, class table, class instance
        if (!lua_istable(L, -1))
        {
            LDC_LOG_ERR("ret top no table!"<<className);
            lua_pop(L, 1);
            return false;
        }

        lua_rotate(L,-2,1); // Stack: class instance, _G, class table
        lua_pop(L, 1); // Stack: class instance

        return true;
    }

    bool static GetTableFromG(lua_State* L, const char* className)
    {
        lua_getglobal(L, className); // Stack: _G, class table
        if (!lua_istable(L, -1))
        {
            LDC_LOG_ERR("top no table!"<<className);
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    // 创建类型
    static void ParseTypeRecursive(const std::vector<std::string>& str_tb,\
        int head, int tail, int& type_end, std::vector<Type>& ret);

    static bool CheckTypeWT(const std::string& type, const EWireType wt)
    {
        const EValueType vt = GetValueType(type);
        const EWireType trueWt = GetWireType(vt);
        return (trueWt == wt);
    }

    static int CheckLuaFuncExist(lua_State* L, int idx, const char* func)
    {
        lua_getfield(L, idx, func);
        int ret = lua_isfunction(L, -1);
        lua_pop(L, 1);
        return ret;
    }

    static SSyntax* ObtainSSyntax(lua_State* L, int idx)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, idx);
        if (!lua_islightuserdata(L, -1))
        {
            return nullptr;
        }

        SSyntax* pSyntax = static_cast<SSyntax*>(lua_touserdata(L,-1));
        lua_pop(L, 1);
        return pSyntax;
    }

    static const SMessage* GetMessageByName(const SSyntax& syntax, const std::string& name)
    {
        if (name.empty() || (name == ""))
        {
            LDC_LOG_ERR("class name empty");
            return nullptr;
        }

        auto iter = syntax.m_messages.find(name);
        if (iter == syntax.m_messages.end())
        {
            LDC_LOG_ERR("class not exist,"<<name);
            return nullptr;
        }

        return &iter->second;
    }

    // 检测继承关系
    static bool CheckInheritance(const SSyntax& syntax, const std::string& sonName, 
        const std::string& baseName)
    {
        const SMessage* msgSon = GetMessageByName(syntax, sonName);
        if (! msgSon)
        {
            return false;
        }

        if (msgSon->m_name == baseName)
        {
            return true;
        }

        // 到Object类为止,不再继续回溯
        if (msgSon->m_name == STR_OBJECT)
        {
            LDC_LOG_ERR("ins to Object");
            return false;
        }
        
        // 不一定每个类都会继承Object,错误日志是为了追查
        if (msgSon->m_base.empty())
        {
            LDC_LOG_ERR("no super, "<< msgSon->m_name);
            return false;
        }

        // 尾递归
        return CheckInheritance(syntax, msgSon->m_base, baseName);
    }

    static bool InitErrFunc(lua_State* L)
    {
        if (!lua_isfunction(L, -1))
        {
            LDC_LOG_ERR("InitLogFunc L->top not func\n");
            return false;
        }

        lua_rawseti(L, LUA_REGISTRYINDEX, LDC_RIDX_FUNC_ERR);
        return true;
    }

    static bool GetErrFunc(lua_State* L)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, LDC_RIDX_FUNC_ERR);
        if (!lua_isfunction(L, -1))
        {
            printf("fatal: GetLogFunc L->top not func\n");
            lua_pop(L, 1);
            return false;
        }

        return true;
    }

    // 栈顶应为类的内存对象,因出错后处理,内部将不再维护堆栈平衡,执行后对象将被丢弃
    static void ProcessErrFunc(lua_State* L)
    {
        if (!GetErrFunc(L))
        {
            return;
        }

        lua_rotate(L, -2, 1);
        lua_call(L, 1, 0); // 不使用保护模式
    }
};

// 辅助检测字节流剩余容量
#define CHECK_INST_LEFT(len, inStream, ext)	    \
do {										    \
    int bleft = inStream.BytesLeft();		    \
    if (bleft < len) {					        \
        LDC_LOG_ERR("read len out range!"		\
            << ", left=" << bleft			    \
            << ", len=" << len				    \
            << ", " << ext);				    \
            return false;					    \
    }										    \
} while (0);


// 拿到语法森林并校验输出
#define DEF_OBTAIN_SSYNTAX(L, idx, pSyntax)                     \
do {                                                            \
    pSyntax = MSeriaHelper::ObtainSSyntax(L, idx);              \
    if (! pSyntax) {                                            \
        lua_pushfstring(L, "ERROR: syntax data is null!");      \
        lua_pushnil(L);                                         \
        return 2;                                               \
    }                                                           \
} while (0);

#define TEMPINSTREAM(inStream)  \
    char* bytes = (char*)inStream.GetBufferBytes(); \
    bytes += inStream.GetOffset(); \
    CBorrowedStreamBuf buf(bytes, len); \
    CIStream tempis(buf); \
    NetInStream tempInStream(tempis);

}