
#include "seria_decode.h"

namespace _LUA_DC
{

bool MSeriaDecode::DecodeNetMessage(lua_State* L, const SSyntax& syntax)
{
    LDC_LOG_DEBUG(__FUNCTION__);

    if (6 != lua_gettop(L))
    {
        LDC_LOG_ERR("lua stack size not 6!");
        lua_pushfstring(L, "lua stack size not 6!");
        lua_pushnil(L);
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TSTRING); 				// class name
    LDC_TYPE_CHECK(L, 2, LUA_TBOOLEAN);				// is encry
    LDC_TYPE_CHECK(L, 3, LUA_TNUMBER);				// encry key
    LDC_TYPE_CHECK(L, 4, LUA_TLIGHTUSERDATA);		// encode's buffer pointer
    LDC_TYPE_CHECK(L, 5, LUA_TNUMBER);				// buffer len
    LDC_TYPE_CHECK(L, 6, LUA_TTABLE);				// decode to table

    const char* classname = lua_tostring(L, 1);
    LDC_LOG_DEBUG("classname:" << classname);

    auto iter = syntax.m_messages.find(classname);
    if (syntax.m_messages.end() == iter)
    {
        LDC_LOG_ERR("not class," << classname);
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: no class : %s", classname);
        return false;
    }

    const SMessage& message = iter->second;
    LDC_LOG_DEBUG(message.m_name);

    const void* value = lua_topointer(L, 4);
    int len = (int) lua_tointeger(L, 5);

    LDC_LOG_DEBUG("DECODE STR LEN:"<<len);

    NetInStream inStream((void*)value, 0, (int)len);
    try 
    {
        if (! FromStream(L, inStream, message, syntax, (1==lua_toboolean(L, 2)), (uint32)lua_tonumber(L, 3)))
        {
            MSeriaHelper::ProcessErrFunc(L);

            LDC_LOG_ERR("FromStream err!"<<message.m_name);
            lua_pushnil(L);
            lua_pushfstring(L, "ERROR: FromStream %s", message.m_name.c_str());
            return false;
        }
    }
    catch (stream_error& e)
    {
        MSeriaHelper::ProcessErrFunc(L);

        LDC_LOG_ERR("LoadItemFromStream "<< message.m_name<<" exception: "<<e.code);
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: FromStream Exception %d", e.code);
        return false;
    }

    lua_pushboolean(L, 1);
    lua_pushboolean(L, 1);
    return true;
}

bool MSeriaDecode::DecodeMemoryObject(lua_State* L, const SSyntax& syntax)
{
    LDC_LOG_DEBUG(__FUNCTION__);

    if (3 != lua_gettop(L)) // 参数个数错误
    {
        LDC_LOG_ERR("lua stack size not 3!");
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: lua stack size not 3!");
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TSTRING); 	// class name
    LDC_TYPE_CHECK(L, 2, LUA_TSTRING);	// encode's buffer
    LDC_TYPE_CHECK(L, 3, LUA_TTABLE);	// decode to table

    const char* classname = lua_tostring(L, 1);
    LDC_LOG_DEBUG("classname:" << classname);

    size_t len = 0;
    const char* value = luaL_checklstring(L, 2, &len);
    if ((NULL == value) || (len <= 0))
    {
        LDC_LOG_ERR("get encode buffer err!"<<len);
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: get encode buffer err! %d", len);
        return false;
    }

    NetInStream inStream((void*)value, 0, (int)len);

    return LoadStream(L, inStream, classname, syntax, false, true, true, true);
}

bool MSeriaDecode::RawUnSerialize(lua_State* L, const SSyntax& syntax)
{
    LDC_LOG_DEBUG(__FUNCTION__);

    if (3 != lua_gettop(L)) // 参数个数错误
    {
        LDC_LOG_ERR("lua stack size not 3!");
        lua_pushnil(L);
        lua_pushfstring(L, "ERROR: lua stack size not 3!");
        return false;
    }

    LDC_TYPE_CHECK(L, 1, LUA_TLIGHTUSERDATA); 	// inStream
    LDC_TYPE_CHECK(L, 2, LUA_TSTRING); 			// class name
    LDC_TYPE_CHECK(L, 3, LUA_TTABLE);			// decode to table

    const char* classname = lua_tostring(L, 2);
    LDC_LOG_DEBUG("classname:" << classname);

    NetInStream* pInStream = (NetInStream*) lua_touserdata(L, 1);
    if (pInStream == nullptr)
    {
        LDC_LOG_ERR("RawUnSerialize pInStream is nullptr!"<<classname);
        lua_pushnil(L);
        lua_pushfstring(L, "RawUnSerialize pInStream is nullptr! %s", classname);
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

    return LoadStream(L, *pInStream, classname, syntax, true, true, true, is_first);
}

bool MSeriaDecode::DecodeCompoundByDesc(lua_State* L,
    NetInStream& inStream, const SMessage& message, const SSyntax& syntax, bool isMem)
{
    UNUSED(isMem);
    LDC_LOG_DEBUG("DecodeCompoundByDesc offSet = "<<inStream.GetOffset()<<", left = "<<inStream.BytesLeft());

    bool IsFather = false;

    while (inStream.BytesLeft() > 1)
    {
        uint16 value = static_cast<uint16>(FromVarUInt(inStream));
        LDC_LOG_DEBUG(value);		

        if (EWireType::DelBase == value)
        {
            IsFather = true;
            break;
        }

        int index = value >> 3;
        LDC_LOG_DEBUG(index);
        const EWireType readWt = (EWireType) (value & 0x0007);
        LDC_LOG_DEBUG(readWt);

        auto iter = message.m_fields.find(index);
        if (message.m_fields.end() == iter) // 兼容反序列化端未定义该数据
        {
            if (! SkipItemFromStream(inStream, readWt)) // 字节流偏移
            {
                LDC_LOG_ERR("skip item err!"
                    <<", index="<<index
                    <<", readWt="<<readWt
                    <<", message="<<message.m_name
                    );
                return false;
            }
            continue;
        }

        const SField& field = iter->second;
        const Type& type = field.m_type;
        LDC_LOG_DEBUG(field.m_name);

        // check wt
        const std::string& vst = type.type;
        if (! MSeriaHelper::CheckTypeWT(vst, (EWireType)readWt))
        {
            LDC_LOG_ERR("wt not match! " << readWt << ", " << vst
                <<", index="<<index
                <<", message="<<message.m_name
                );
            return false;
        }
        
        // 读出的数据已经成功压栈顶, 才能赋值
        if (!DecodeItem(index, L, inStream, type, syntax))
        {
            LDC_LOG_ERR("DecodeItem fail, index="<<index<<", message="<<message.m_name);
            return false;
        }

        lua_setfield(L, -2, field.m_name.c_str());
    }

    if (IsFather && (! message.m_base.empty()))
    {
        const std::string& base = message.m_base;
        auto iter = syntax.m_messages.find(base);
        if (syntax.m_messages.end() == iter)
        {
            LDC_LOG_WARN("no base=" << base
                <<", message="<< message.m_name); 
            return true; // 容错基类未构建入语法树
        }

        const SMessage& smessage = iter->second;
        LDC_LOG_DEBUG(smessage.m_name);
        LDC_LOG_DEBUG("DecodeCompoundByDesc Base offSet = "<<inStream.GetOffset()<<", left = "<<inStream.BytesLeft());

        bool isProcessBySelfDef = false;
        if (! DecodeBaseBySelfDef(L, inStream, smessage.m_name.c_str(), isProcessBySelfDef) )
        {
            return false;
        }

        if (! isProcessBySelfDef)
        {
            return LoadCompoundByDesc(L, inStream, smessage, syntax, true, true, false);
        }
    }

    return true;
}

bool MSeriaDecode::DecodeItem(int fsIndex, lua_State* L, NetInStream& inStream,
    const Type& type, const SSyntax& syntax)
{
    LDC_LOG_DEBUG("DecodeItem, offset = "<<inStream.GetOffset()<<", left = "<<inStream.BytesLeft());

    const std::string& st = type.type;

    const EValueType vt = MSeriaHelper::GetValueType(st);
    const EWireType wt = MSeriaHelper::GetWireType(vt);

    switch (vt)
    {
        case EValueType::Int32:
        {
            int value = -1;
            if (! LoadProtobuf(fsIndex, value, inStream, wt))
            {
                return false;
            }

            LDC_LOG_DEBUG(value);

            if ("bool" == st)
            {
                bool bvalue = value > 0;
                lua_pushboolean(L, bvalue);
            }
            else
            {
                if ((INT32_MIN > value) || (INT32_MAX < value))
                {
                    LDC_LOG_ERR("int value beyond, "<<value);
                    return false;
                }

                lua_pushinteger(L, (lua_Integer)value);
            }			
        }
        break;

        case EValueType::Int64:
        {
            int64 value = -1;
            if (! LoadProtobuf(fsIndex, value, inStream, wt))
            {
                return false;
            }

            if ((INT64_MIN > value) || (INT64_MAX < value))
            {
                LDC_LOG_ERR("int64 value beyond, "<<value);
                return false;
            }

            LDC_LOG_DEBUG(value);
            lua_pushinteger(L, (lua_Integer)value);
        }
        break;

        case EValueType::Float32:
        {
            float value = inStream.ReadFloat();
            
            if (((-3.4E+38 - value) > 0.000001 ) || ((value - 3.4E+38) > 0.000001))
            {
                LDC_LOG_ERR("float value beyond, "<<value);
                return false;
            }

            LDC_LOG_DEBUG(value);
            lua_pushnumber(L, (lua_Number)value);
        }
        break;

        case EValueType::Float64:
        {
            float64 value = inStream.ReadDouble();

            if (((-1.79E+308 - value) > 0.000001 ) || ((value - 1.79E+308) > 0.000001))
            {
                LDC_LOG_ERR("float64 value beyond, "<<value);
                return false;
            }

            LDC_LOG_DEBUG(value);
            lua_pushnumber(L, (lua_Number)value);
        }
        break;

        case EValueType::String:
        {
            std::string value;
            if (! LoadProtobuf(fsIndex, value, inStream))
            {
                return false;
            }

            LDC_LOG_DEBUG(value);
            lua_pushstring(L, value.c_str());
        }
        break;

        case EValueType::ENetBuffer:
        {
            NetBuffer value;
            if (! LoadProtobuf(fsIndex, value, inStream))
            {
                return false;
            }

            lua_pushlstring(L, (const char*)value.GetBuffer(), value.GetLength());
        }
        break;

        case EValueType::Time64:
        {
            int64 value = -1;
            if (! LoadProtobuf(fsIndex, value, inStream, wt))
            {
                return false;
            }

            LDC_LOG_DEBUG(value);
            lua_pushinteger(L, (lua_Integer)value);
        }
        break;

        case EValueType::List:
        {
            lua_createtable(L, 0, 0);
            return LoadProtobufList(fsIndex, L, inStream, type, syntax);
        }
        break;

        case EValueType::Map:
        {
            lua_createtable(L, 0, 0);
            return LoadProtobufMap(fsIndex, L, inStream, type, syntax);
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

            return LoadProtobufCompound(L, inStream, smessage, syntax);
        }
        break;

        case EValueType::Byte:
        {
            int value = -1;
            if (! LoadProtobuf(fsIndex, value, inStream, wt))
            {
                return false;
            }

            if ((INT8_MIN > value) || (INT8_MAX < value))
            {
                 LDC_LOG_ERR("byte value beyond, "<<value);
            }

            LDC_LOG_DEBUG(value);
            lua_pushinteger(L, (lua_Integer)value);
        }
        break;

        case EValueType::Any:
        {
            return LoadProtobufAny(fsIndex, L, inStream, syntax);
        }
        break;

        case EValueType::Reference:
        {
            std::string baseName = st;
            baseName.pop_back();
            LDC_LOG_DEBUG("load Reference,"<<baseName);
            return LoadProtobufReference(fsIndex, L, inStream, syntax, baseName);
        }
        break;

        default:
        {
            LDC_LOG_ERR("err type!" << st);
            return false;
        }
        break;
    }

    return true;
}

}