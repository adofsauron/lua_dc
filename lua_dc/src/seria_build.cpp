#include "seria_build.h"

namespace _LUA_DC
{

bool MSeriaBuild::BuildMessage(lua_State* L, SSyntax& syntax, const int off)
{
    LDC_TYPE_CHECK(L, 1, LUA_TTABLE);
    
    const char* name {nullptr};
    LUA_STACK_KEY(L, STR_CLASSNAME, off);
    {
        LDC_TYPE_CHECK(L, -1, LUA_TSTRING);
        name = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    if (! name)
    {
        LDC_LOG_ERR("no class name, "<< name);
        lua_pushnil(L);
        lua_pushfstring(L, "Error: no class name, %s", name);
        return false;
    }

    bool lbWrite = true;
    // 检测_proto属性
    lua_pushstring(L, STR_PROTO);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1))
    {
        lbWrite = false;
        LDC_LOG_DEBUG("object class no _proto, "<<name);
    }
    lua_pop(L, 1);

    // 已创建
    auto iter = syntax.m_messages.find(name);
    if (iter != syntax.m_messages.end())
    {
        LDC_LOG_DEBUG("has message : "<<name);
        lua_pushboolean(L, 1);
        lua_pushfstring(L, "NOTE: has message %s", name);
        return true;
    }

    LDC_LOG_DEBUG("build message," << name);

    SMessage message;
    message.m_name = name;
    
    // proto
    LUA_STACK_KEY(L, STR_PROTO, off);
    {
        if ( LUA_TNIL != lua_type(L, -1) )
        {
            MSeriaBuild::BuildField(L, message);
        }
    }
    lua_pop(L, 1);

    // super
    LUA_STACK_KEY(L, STR_SUPER, off);
    do {
        if ( LUA_TNIL == lua_type(L, -1))
        {
            break;
        }

        LDC_TYPE_CHECK(L, -1, LUA_TTABLE);
        LUA_STACK_KEY(L, STR_CLASSNAME, -2);
        {
            LDC_TYPE_CHECK(L, -1, LUA_TSTRING);
            const char* super = lua_tostring(L, -1);
            LDC_LOG_DEBUG("super is "<<super);
            message.m_base = super;
        };
        lua_pop(L, 1);

        // 确保堆栈一致
        if (! message.m_base.empty())
        {
            if (! MSeriaBuild::BuildFather(L, syntax))
            {
                LDC_LOG_ERR("Build Base fail");
                lua_pushnil(L);
                lua_pushfstring(L, "ERROR: BuildFather fail, %s -> %s", name, message.m_base.c_str());
                return false;
            }

            LDC_LOG_DEBUG("Build Base succ,"<<message.m_name<<" -> "<<message.m_base);
        }
    }  while (0);
    lua_pop(L, 1);

    // 兼容
    if (! lbWrite)
    {
        message.m_fields.clear();
        message.m_keys.clear();   
    }

    // 至此才可用值拷贝的方式加入语法森林
    syntax.m_messages.emplace(message.m_name, message);
    LDC_LOG_DEBUG("build "<<message.m_name<<" success");
    lua_pushboolean(L, 1);
    lua_pushfstring(L, "NOTE: builde success, %s ", name);
    return true;
}

bool MSeriaBuild::BuildMessage(lua_State* L, SSyntax& syntax)
{
    return BuildMessage(L, syntax, 1);
}

bool MSeriaBuild::BuildField(lua_State* L, SMessage& message)
{
     LDC_TYPE_CHECK(L, 1, LUA_TTABLE);

    std::set<int>& keys = message.m_keys;

    int idx = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, idx))
    {
        LDC_TYPE_CHECK(L, -2, LUA_TNUMBER);
        keys.emplace((int)lua_tonumber(L, -2));
        lua_pop(L, 1);
    }

    if (keys.empty())
    {
        LDC_LOG_DEBUG("keys empty,"<< message.m_name);
        return true;
    }

    const char* name{nullptr};
    static char type[1024] = {0x00};
    std::map<int,SField>& fileds = message.m_fields;
    for (int i:keys)
    {
        lua_pushinteger(L, i);
        lua_gettable(L, -2);
        {
            lua_pushinteger(L, 1);
            lua_gettable(L, -2);
            {
                LDC_TYPE_CHECK(L, -1, LUA_TSTRING);
                name = lua_tostring(L, -1);
                if (! name)
                {
                    LDC_LOG_ERR("name null");
                    return false;
                }
            };
            lua_pop(L, 1);

            lua_pushinteger(L, 2);
            lua_gettable(L, -2);
            {
                if (!lua_isstring(L, -1))
                {
                    LDC_LOG_ERR("not string! i="<<i
                        <<",type="<<lua_type(L,-1)
                        <<", name="<< name
                        <<", mname=" << message.m_name
                        );
                    return false;
                }

                char* p = (char*)lua_tostring(L, -1);
                if (! p)
                {
                    LDC_LOG_ERR("type null");
                    return false;
                }
                memset(type, 0x00, 1024);
                memcpy(type, lua_tostring(L, -1), strlen(p));
            };
            lua_pop(L, 1);
        };
        lua_pop(L, 1);

        // 确保name和type都能正确取到再创建filed
        SField& field = fileds[i];
        field.m_name = name;
        field.m_index = i;

        // 去除空格
        MTools::DelChar(type, ' ');
        // 兼容做法
        static std::string str;
        str = type;
        if (! MTools::IsAvailString(str))
        {
            LDC_LOG_ERR("unavial char type:"<<str);
            return false;
        }

        std::vector<std::string> str_tb;
        MTools::SplitStr2Tb(str,str_tb);
        int type_end = -1;
        std::vector<Type> types;
        MSeriaHelper::ParseTypeRecursive(str_tb, 0, (int)str_tb.size(), type_end, types);
        if (types.empty())
        {
            LDC_LOG_ERR("ParseTypeRecursive empty");
            return false;
        }

        field.m_type = *types.begin();
    }

    return true;
}

bool MSeriaBuild::BuildFather(lua_State* L, SSyntax& syntax)
{
    return BuildMessage(L, syntax, -2);
}

bool MSeriaBuild::BuildCheckFather(SSyntax& syntax)
{
    if (syntax.m_messages.empty())
    {
        return true;
    }

    // 检测基类引用可用性
    for (const auto& value:syntax.m_messages)
    {
        const auto& message = value.second;
        if (message.m_base.empty())
        {
            continue;
        }

        if ( syntax.m_messages.end() == syntax.m_messages.find(message.m_base) )
        {
            LDC_LOG_ERR("no super,"<<message.m_name<<","<<message.m_base);
            return false;
        }
    }

    return true;
}

bool MSeriaBuild::BuildRefMap(lua_State* L, SSyntax& syntax)
{
    if (1 != lua_gettop(L))
    {
        LDC_LOG_ERR("lua stack size not 1!");
        lua_pushnil(L);
        lua_pushfstring(L, "lua stack size not 1!");
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TTABLE);

    int size = 0;
    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        ++size;
        lua_pop(L, 1);
    }

    if (size == 0)
    {
        lua_pushboolean(L, 1);
        lua_pushfstring(L, "NOTE: BuildRefMap success, size = %d ", size);
        return true;
    }

    // 函数会重入?
    syntax.m_ref_name.clear();
    syntax.m_ref_id.clear();

    syntax.m_ref_name.reserve(size);
    syntax.m_ref_id.reserve(size);

    constexpr int errMsgLen = 128;
    char errMsg[errMsgLen] = {0};

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        lua_pushvalue(L, -2); // stack...top -> key

        if (! lua_isstring(L, -1))
        {
            snprintf(errMsg, errMsgLen, "key is not string, but type is %s", lua_typename(L, lua_type(L, -1)));

            LDC_LOG_ERR(errMsg);
            lua_pushnil(L);
            lua_pushfstring(L, errMsg);
            return false;
        }
        
        std::string classname = lua_tostring(L,-1);
        LDC_LOG_DEBUG("classname=" << classname);

        lua_pop(L, 1); // stack top -> value

        if (! lua_isinteger(L, -1))
        {
            snprintf(errMsg, errMsgLen, "value is not int, but type is %s, classname[%s]", 
                lua_typename(L, lua_type(L, -1)), classname.c_str());

            LDC_LOG_ERR(errMsg);
            lua_pushnil(L);
            lua_pushfstring(L, errMsg);
            return false;
        }

        lua_Integer lua_value = lua_tointeger(L, -1);
        if ((INT32_MIN > lua_value) || (INT32_MAX < lua_value))
        {
            snprintf(errMsg, errMsgLen, "value[%lld] is beyond integer, classname[%s]", lua_value, classname.c_str());

            LDC_LOG_ERR(errMsg);
            lua_pushnil(L);
            lua_pushfstring(L, errMsg);
            return false;
        }

        if (lua_value < 0) // 不能小于0
        {
            snprintf(errMsg, errMsgLen, "value[%d] < 0, classname[%s]", (int)lua_value, classname.c_str());

            LDC_LOG_ERR(errMsg);
            lua_pushnil(L);
            lua_pushfstring(L, errMsg);
            return false;
        }
        
        uint32 value = (uint32) (lua_value);
        LDC_LOG_DEBUG("VALUE=" << value);

        auto ref_name_ret = syntax.m_ref_name.emplace(classname, value);
        if (!ref_name_ret.second)
        {
            snprintf(errMsg, errMsgLen, "ref classname repetition! classname[%s]", classname.c_str());

            LDC_LOG_ERR(errMsg);
            lua_pushnil(L);
            lua_pushfstring(L, errMsg);
            return false;
        }
        
        auto ref_id_ret = syntax.m_ref_id.emplace(value, ref_name_ret.first);
        if (!ref_id_ret.second)
        {
            snprintf(errMsg, errMsgLen, "ref id repetition! classname[%s] value[%d]", classname.c_str(), value);

            LDC_LOG_ERR(errMsg);
            lua_pushnil(L);
            lua_pushfstring(L, errMsg);
            return false;
        }

        lua_pop(L, 1);
    }

    LDC_LOG_DEBUG("BuildRefMap ref size = "<<size);

    lua_pushboolean(L, 1);
    lua_pushfstring(L, "NOTE: BuildRefMap success, size = %d ", size);
    return true;
}

}