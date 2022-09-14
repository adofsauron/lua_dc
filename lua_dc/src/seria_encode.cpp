#include "seria_encode.h"

namespace _LUA_DC
{

bool MSeriaEncode::EncodeNetMessage(lua_State* L, const SSyntax& syntax)
{
    LDC_LOG_DEBUG(__FUNCTION__);

    if (4 != lua_gettop(L)) // 参数个数错误
    {
        LDC_LOG_ERR("lua stack size not 4!");
        lua_pushfstring(L, "lua stack size not 4!");
        lua_pushnil(L);
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TSTRING);		// class name
    LDC_TYPE_CHECK(L, 2, LUA_TBOOLEAN);		// is encry
    LDC_TYPE_CHECK(L, 3, LUA_TNUMBER);		// encry key
    LDC_TYPE_CHECK(L, 4, LUA_TTABLE);		// table to encode

    const char* classname = lua_tostring(L,1);

    LDC_LOG_DEBUG("classname:"<<classname);

    auto iter = syntax.m_messages.find(classname);
    if (syntax.m_messages.end() == iter)
    {
        LDC_LOG_ERR("not class,"<<classname);
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: no class : %s", classname);
        return false;
    }

    const SMessage& message = iter->second;
    LDC_LOG_DEBUG(message.m_name);

    CDynamicStreamBuf buf;
    COStream ostream(buf);
    NetOutStream outStream(ostream);

    if (! ToStream(L, outStream, message, syntax, (1==lua_toboolean(L,2)), (uint32)lua_tonumber(L,3)))
    {
        LDC_LOG_ERR("ToStream fail,"<<classname);
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: ToStream fail : %s", classname);
        return false;
    }

    int len = outStream.GetBufferLength();
    void* bytes = outStream.GetBufferBytes();

    LDC_LOG_DEBUG("ENCODE LEN:"<<len);
    lua_pushboolean(L, 1);
    lua_pushlstring(L, (const char*)bytes, len);

    return true;
}

bool MSeriaEncode::EncodeMemoryObject(lua_State* L, const SSyntax& syntax)
{
    LDC_LOG_DEBUG(__FUNCTION__);

    if (2 != lua_gettop(L)) // 参数个数错误
    {
        LDC_LOG_ERR("lua stack size not 2!");
        lua_pushnil(L);
        lua_pushfstring(L, "lua stack size not 2!");
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TSTRING);		// class name
    LDC_TYPE_CHECK(L, -1, LUA_TTABLE);		// table to encode

    const char* classname = lua_tostring(L,1);

    CDynamicStreamBuf buf;
    COStream ostream(buf);
    NetOutStream outStream(ostream);

    if (! WrapSaveStream(L, outStream, classname, syntax, false, true, true, true))
    {
        LDC_LOG_ERR("RawSerialize WrapSaveStream is fail!"<<classname);
        return false;
    }

    return true;
}

bool MSeriaEncode::RawSerialize(lua_State* L, const SSyntax& syntax)
{
    LDC_LOG_DEBUG(__FUNCTION__);

    if (3 != lua_gettop(L)) // 参数个数错误
    {
        LDC_LOG_ERR("RawSerialize lua stack size not 3!");
        lua_pushfstring(L, "RawSerialize lua stack size not 3!");
        lua_pushnil(L);
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TLIGHTUSERDATA);		// outStream
    LDC_TYPE_CHECK(L, 2, LUA_TSTRING);				// class name
    LDC_TYPE_CHECK(L, -1, LUA_TTABLE);				// table to encode

    const char* classname = lua_tostring(L,2);

    NetOutStream* pOutStream = (NetOutStream*) lua_touserdata(L, 1);
    if (pOutStream == nullptr)
    {
        LDC_LOG_ERR("RawSerialize pOutStream is nullptr!"<<classname);
        lua_pushfstring(L, "RawSerialize pOutStream is nullptr! %s", classname);
        lua_pushnil(L);
        return false;
    }

	const char* name{ nullptr };
	LUA_STACK_KEY(L, STR_CLASSNAME, -2);
	{
		LDC_TYPE_CHECK(L, -1, LUA_TSTRING);
		name = lua_tostring(L, -1);
	}
	lua_pop(L, 1);

	bool is_first = memcmp(name, classname, strlen(name)) == 0;
    
    LDC_LOG_DEBUG("RawSerialize offSet = "<<pOutStream->GetOffset());
    if (! WrapSaveStream(L, *pOutStream, classname, syntax, true, true, true, is_first))
    {
        LDC_LOG_ERR("RawSerialize WrapSaveStream is fail!"<<classname);
        return false;
    }

    return true;
}

bool MSeriaEncode::EncodeItem(int fsIndex, lua_State* L, NetOutStream& outStream, const SSyntax& syntax,
    const Type& type)
{
    LDC_LOG_DEBUG("EncodeItem offSet = "<<outStream.GetOffset());

    const std::string& st = type.type;

    const EValueType vt = MSeriaHelper::GetValueType(st);
    const EWireType wt = MSeriaHelper::GetWireType(vt);

    switch (vt)
    {
        case EValueType::Int32:
        {
            if ( (!lua_isboolean(L, -1)) && (!lua_isinteger(L, -1)) )
            {
                LDC_LOG_ERR("type not boolean or int!"<<(lua_typename(L, lua_type(L, -1))));
                return false;
            }

            lua_Integer lua_value = (LUA_TBOOLEAN == lua_type(L, -1)) ?
                lua_toboolean(L, -1) :
                lua_tointeger(L, -1);

            if (lua_isboolean(L, -1) && (lua_value != 1) && (lua_value != 0))
            {
                LDC_LOG_ERR("bool value not 1 or 0, "<<lua_value);
                return false;
            }

            if ((INT32_MIN > lua_value) || (INT32_MAX < lua_value))
            {
                LDC_LOG_ERR("int value beyond, "<<lua_value);
                return false;
            }
            
            int value = (int) lua_value;
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, outStream, wt);
        }
        break;

        case EValueType::Int64:
        {
            if (! lua_isinteger(L, -1))
            {
                LDC_LOG_ERR("not int64," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            lua_Integer lua_value = lua_tointeger(L, -1);
            if ((INT64_MIN > lua_value) || (INT64_MAX < lua_value))
            {
                LDC_LOG_ERR("int64 value beyond, "<<lua_value);
                return false;
            }

            int64 value = (int64)lua_tointeger(L, -1);
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, outStream, wt);
        }
        break;

        case EValueType::Float32:
        {
            if (! lua_isnumber(L, -1))
            {
                LDC_LOG_ERR("not number," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            lua_Number lua_value = lua_tonumber(L, -1);
            if (((-3.4E+38 - lua_value) > 0.000001 ) || ((lua_value - 3.4E+38) > 0.000001))
            {
                LDC_LOG_ERR("float value beyond, "<<lua_value);
                return false;
            }

            float value = (float) lua_value;
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, outStream);
        }
        break;

        case EValueType::Float64:
        {
            if (! lua_isnumber(L, -1))
            {
                LDC_LOG_ERR("not number," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            lua_Number lua_value = lua_tonumber(L, -1);
            if (((-1.79E+308 - lua_value) > 0.000001 ) || ((lua_value - 1.79E+308) > 0.000001))
            {
                LDC_LOG_ERR("float64 value beyond, "<<lua_value);
                return false;
            }

            float64 value = (float64) lua_value;
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, outStream);
        }
        break;

        case EValueType::String:
        {
            if (! lua_isstring(L, -1))
            {
                LDC_LOG_ERR("not string," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            size_t len = 0;
            const char* value = lua_tolstring(L, -1, &len);
            len += 1;
            LDC_LOG_DEBUG(len);
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, len, outStream);
        }
        break;

        case EValueType::ENetBuffer:
        {
            if (! lua_isstring(L, -1))
            {
                LDC_LOG_ERR("not buffer string," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            size_t len = 0;
            const char* value = luaL_checklstring(L, -1, &len);
            LDC_LOG_DEBUG(len);
            if ((value != NULL) && (len > 0)) // 兼容
            {
                NetBuffer buffer((void*)value, (int)len);
                SaveProtobuf(fsIndex, buffer, len, outStream);
            }
        }
        break;

        case EValueType::Time64:
        {
            if (! lua_isinteger(L, -1))
            {
                LDC_LOG_ERR("not number," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            lua_Integer lua_value = lua_tointeger(L, -1);
            if ((INT64_MIN > lua_value) || (INT64_MAX < lua_value))
            {
                LDC_LOG_ERR("Time64 value beyond, "<<lua_value);
                return false;
            }

            int64 value = (int64) lua_value;
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, outStream, wt);
        }
        break;

        case EValueType::Compound:
        {
            const std::string& lstrCons = type.type;
            auto iter = syntax.m_messages.find(lstrCons);
            if (syntax.m_messages.end() == iter)
            {
                LDC_LOG_ERR("not class," << lstrCons);
                return false;
            }

            const SMessage& smessage = iter->second;
            LDC_LOG_DEBUG(smessage.m_name);

            return SaveProtobufCompound(fsIndex, L, outStream, smessage, syntax, false, true, false, false);
        }
        break;

        case EValueType::List:
        {
            return SaveProtobufList(fsIndex, type, L, outStream, syntax);
        }
        break;

        case EValueType::Map:
        {
            return SaveProtobufMap(fsIndex, type, L, outStream, syntax);
        }
        break;

        case EValueType::Byte:
        {
            if (! lua_isinteger(L, -1))
            {
                LDC_LOG_ERR("byte type not int," << (lua_typename(L, lua_type(L, -1))));
                return false;
            }

            lua_Integer lua_value = lua_tointeger(L, -1);
            if ((INT8_MIN > lua_value) || (INT8_MAX < lua_value))
            {
                LDC_LOG_ERR("byte value beyond, "<<lua_value);
                return false;
            }

            int value = (int) lua_value;
            LDC_LOG_DEBUG("VALUE=" << value);
            SaveProtobuf(fsIndex, value, outStream, wt);
        }
        break;

        case EValueType::Any:
        {
            if (! lua_istable(L, -1))
            {
                LDC_LOG_ERR("Any value not table");
                return false;
            }

            return SaveProtobufAny(fsIndex, L, outStream, syntax);
        }

        case EValueType::Reference:
        {
            if (! lua_istable(L, -1))
            {
                LDC_LOG_ERR("Reference value not table");
                return false;
            }

            std::string baseName = st;
            baseName.pop_back();
            LDC_LOG_DEBUG("save Reference,"<<baseName);
            return SaveProtobufReference(fsIndex, L, outStream, syntax, baseName);
        }

        default:
        {
            LDC_LOG_ERR("unkown type:"<<st);
            return false;
        }
    }

    return true;
}

    // L(-1)=>Seria,正确处理则改变堆栈层级,错误则原样返回
    bool MSeriaEncode::EncodeCompoundOptionSelfDef(lua_State* L, NetOutStream& outStream,
        const SMessage& message, const SSyntax& syntax, bool byBase, bool writeLen, bool isMem, bool isFirst)
    {
        if (lua_isnil(L, -1)) // 描述式
        {
            if (byBase)
            {
                lua_pop(L, 2);
            }
            else
            {
                lua_pop(L, 1);
            }

            if (isFirst)
            {
                return EncodeCompoundByDescWithLen(L, outStream, message, syntax, isMem);
            }
            
            if (isMem)
            {
                return EncodeCompoundByDesc(L, outStream, message, syntax, isMem);
            }

            if (writeLen)
            {
                return EncodeCompoundByDescWithLen(L, outStream, message, syntax, isMem);
            }
            
            return EncodeCompoundByDesc(L, outStream, message, syntax, isMem);
        }
        
        // 自定义式
        if (! lua_isfunction(L, -1))
        {
            LDC_LOG_ERR("no Serialize func, class="<<message.m_name<<",type="<<lua_type(L,-1));
            return false;
        }

		if (byBase)
		{
			lua_pushvalue(L, -3);
		}
		else
		{
			lua_pushvalue(L, -2);
		}

        lua_pushlightuserdata(L, (void*)&outStream);
        lua_call(L, 2, 0); // 进入自定义反序列化处理,调用方维护成员和基类的序列化
        if (byBase)
        {
            lua_pop(L, 1);
        }
        LDC_TYPE_CHECK(L, -1, LUA_TTABLE);
        
        return true;
    }

}