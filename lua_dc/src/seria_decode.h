#pragma once

#include "seria.h"
#include "def.h"
#include "tools.h"
#include "lua_wrap.h"
#include "seria_helper.h"
#include "encrypt.h"
#include "buffer/NetInStream.h"
#include "buffer/NetBuffer.h"

namespace _LUA_DC
{

class MSeriaDecode
{
public:

    static bool DecodeNetMessage(lua_State* L, const SSyntax& syntax);

    static bool DecodeMemoryObject(lua_State* L, const SSyntax& syntax);

    static bool RawUnSerialize(lua_State* L, const SSyntax& syntax);

private:

    static bool DecodeCompoundByDesc(lua_State* L, NetInStream& inStream,
        const SMessage& message, const SSyntax& syntax, bool isMem);

    static bool DecodeItem(int fsIndex, lua_State* L, NetInStream& inStream,
        const Type& type, const SSyntax& syntax);

    static inline bool DecodeCompoundByDescWithLen(lua_State* L, NetInStream& inStream,
        const SMessage& message, const SSyntax& syntax, bool isMem)
    {
        int len = inStream.ReadInt();
        CHECK_INST_LEFT(len, inStream, message.m_name);
        {
            TEMPINSTREAM(inStream);
            if (! DecodeCompoundByDesc(L, tempInStream, message, syntax, isMem))
            {
                LDC_LOG_ERR("DecodeCompoundByDesc err!" << message.m_name);
                return false;
            }
        }
        inStream.Skip(len);
        return true;
    }

    static inline bool LoadCompoundByDesc(lua_State* L, NetInStream& inStream,
        const SMessage& message, const SSyntax& syntax, bool isReadLen, bool isMem, bool isFirst)
    {
        LDC_LOG_DEBUG("LoadCompoundByDesc offSet = "<<inStream.GetOffset()<<", left ="<<inStream.BytesLeft());

        if (isFirst)
        {
            return DecodeCompoundByDescWithLen(L, inStream, message, syntax, isMem);
        }
        
        if (isMem)
        {
            return DecodeCompoundByDesc(L, inStream, message, syntax, isMem);
        }
        
        if (isReadLen)
        {
            return DecodeCompoundByDescWithLen(L, inStream, message, syntax, isMem);
        }

        if (! DecodeCompoundByDesc(L, inStream, message, syntax, isMem))
        {
            LDC_LOG_ERR("DecodeCompoundByDesc err!" << message.m_name);
            return false;
        }
        
        return true;
    }

    static inline bool LoadCompound(lua_State* L, NetInStream& inStream,
        const SMessage& message, const SSyntax& syntax, bool isRaw = false, bool isReadLen = true, bool isMem = false, bool isFirst = false)
    {
        LDC_LOG_DEBUG("LoadCompound offSet = "<<inStream.GetOffset()<<", left ="<<inStream.BytesLeft());

        if (isRaw)
        {      
            return LoadCompoundByDesc(L, inStream, message, syntax, isReadLen, isMem, isFirst);
        }

        if (! MSeriaHelper::GetTableFromG(L, message.m_name.c_str()) ) // 强制提取该类属性
        {
            LDC_LOG_ERR("MSeriaHelper::GetTableFromG fail, "<<message.m_name);
            return false;
        }

        lua_getfield(L, -1, STR_UNSERIA);

        if (lua_isnil(L, -1)) // 描述式
        {
            lua_pop(L, 2);
            return LoadCompoundByDesc(L, inStream, message, syntax, isReadLen, isMem, isFirst);
        }

        // 自定义式
        lua_pushvalue(L, -3); // 要赋值的idx的tb
        lua_pushlightuserdata(L, (void*)&inStream);
        lua_call(L, 2, 0);
        lua_pop(L, 1); // 删掉提取属性的tb
    
        return true;
    }

    static inline bool FromStream(lua_State* L, NetInStream& inStream, const SMessage& message,
        const SSyntax& syntax, bool decrypt, uint32 key)
    {
        uint32 securityFlag = inStream.ReadUInt();
        if (DEF_SECRITY_FLAG != securityFlag)
        {
            LDC_LOG_ERR("securityFlag err!" << securityFlag << "," << DEF_SECRITY_FLAG);
            return false;
        }

        int checkSum = inStream.ReadInt();
        int buffLength = inStream.ReadInt();
        int buffStart = inStream.GetOffset();
        if (decrypt)
        {
            int c = MEncrypt::GetCRC32((char*)inStream.GetBufferBytes(), buffStart, buffLength);
            if (c != checkSum)
            {
                LDC_LOG_ERR("checkSum err!" << c << "\t" << checkSum);
                throw stream_error(SEC_CHECKSUM_ERROR);
            }
            MEncrypt::Decrypt(key, (char*)inStream.GetBufferBytes(), buffStart, buffLength);
        }

        LDC_LOG_DEBUG(checkSum);
        LDC_LOG_DEBUG(buffLength);
        LDC_LOG_DEBUG(buffStart);

        return LoadCompound(L, inStream, message, syntax, false, true, true, true);;
    }

    static inline int64 FromVarInt(NetInStream& inStream)
    {
        int64 n = 0;
        int shift = 0;
        for (;;)
        {
            uint8 aByte = inStream.ReadByte();
            int64 bvar = aByte & 0x7f;
            n += (bvar << shift);
            shift += 7;
            if (0 == (aByte & 0x80))
            {
                break;
            }
        }

        return ((uint64)n >> 1) ^ (-(n & 1));
    }

    // 对正整数反序列化
    static inline uint64 FromVarUInt(NetInStream& inStream)
    {
        uint8 res = inStream.ReadByte();
        if (!(res & 0x80)) // 注意第一个字节与128比较
        {
            return static_cast<uint64>(res);
        }

        inStream.Seek(inStream.GetOffset()-1); // 回撤1字节

        int64 n = 0;
        int shift = 0;
        for (;;)
        {
            uint8 aByte = inStream.ReadByte();
            int64 bvar = aByte & 0x7f;
            n += (bvar << shift);
            shift += 7;
            if (0 == (aByte & 0x80))
            {
                break;
            }
        }

        return n;
    }

    static inline bool SkipItemFromStream(NetInStream& inStream, EWireType wt)
    {
        switch (wt)
        {
            case EWireType::EVarInt:
            {
                FromVarInt(inStream);
            }
            break;

            case EWireType::Data32:
            {
                inStream.Skip(4);
            }
            break;

            case EWireType::Data64:
            {
                inStream.Skip(8);
            }
            break;

            case EWireType::LengthDelimited:
            {
                inStream.Skip(inStream.ReadInt());
            }
            break;

            case EWireType::LengthDelVarUInt:
            {
                inStream.Skip(static_cast<int>(FromVarUInt(inStream)));
            }
            break;

            default:
            {
                LDC_LOG_ERR("wt unkonw!" << wt);
                return false;
            }
        }

        return true;
    }

    static inline bool LoadProtobuf(int fsIndex, int& value, NetInStream& inStream, EWireType wiretype)
    {
        switch (wiretype)
        {
            case EWireType::Data32:
            {
                value = inStream.ReadInt();
            }
            break;

            case EWireType::EVarInt:
            {
                value = (int)FromVarInt(inStream);
            }
            break;

            default:
            {
                LDC_LOG_ERR("unkown wt!" << wiretype
                    << ", fsIndex=" << fsIndex
                );
                return false;
            }
        }

        return true;
    }

    static inline bool LoadProtobuf(int fsIndex, int64& value, NetInStream& inStream, EWireType wiretype)
    {
        switch (wiretype)
        {
            case EWireType::Data64:
            {
                value = inStream.ReadLong();
            }
            break;

            case EWireType::EVarInt:
            {
                value = FromVarInt(inStream);
            }
            break;

            default:
            {
                LDC_LOG_ERR("unkown wt!" << wiretype
                    << ", fsIndex=" << fsIndex
                );
                return false;
            }
        }

        return true;
    }

    static inline bool LoadProtobuf(int fsIndex, std::string& value, NetInStream& inStream)
    {
        int len = static_cast<int>(FromVarUInt(inStream));
        CHECK_INST_LEFT(len, inStream, fsIndex);
        {
            TEMPINSTREAM(inStream);
            value = (char*)tempInStream.GetBufferBytes();
        }
        inStream.Skip(len);

        return true;
    }

    static inline bool LoadProtobuf(int fsIndex, NetBuffer& value, NetInStream& inStream)
    {
        int len = static_cast<int>(FromVarUInt(inStream));
        if (len <= 0)
        {
            return true;
        }

        CHECK_INST_LEFT(len, inStream, fsIndex);
        {
            char* bytes = (char*)inStream.GetBufferBytes();
            bytes += inStream.GetOffset();
            value.Buffer().Put(bytes, 0, len);
        }
        inStream.Skip(len);
        return true;
    }

    static inline bool LoadProtobufList(int fsIndex, lua_State* L, NetInStream& inStream,
        const Type& type, const SSyntax& syntax)
    {
        if (type.params.empty())
        {
            LDC_LOG_ERR("list params empty!" << fsIndex);
            return false;
        }

        int len = inStream.ReadInt();
        CHECK_INST_LEFT(len, inStream, fsIndex);
        {
            TEMPINSTREAM(inStream);
            // int size = tempInStream.ReadInt();
            int size = static_cast<int>(FromVarUInt(tempInStream));
            if (0 < size)
            {
                tempInStream.ReadByte();
                const Type& vType = type.params[0];
                for (int i = 1; i <= size; ++i)
                {
                    if (! DecodeItem(-1, L, tempInStream, vType, syntax))
                    {
                        return false;
                    }

                    lua_seti(L, -2, i);
                }
            }
        }
        inStream.Skip(len);
        return true;
    }

    static inline bool LoadProtobufMap(int fsIndex, lua_State* L, NetInStream& inStream,
        const Type& type, const SSyntax& syntax)
    {
        if (type.params.size() != 2)
        {
            LDC_LOG_ERR("map params size !=2");
            return false;
        }

        int len = inStream.ReadInt();
        CHECK_INST_LEFT(len, inStream, fsIndex);
        {
            TEMPINSTREAM(inStream);
            // int size = tempInStream.ReadInt();
            int size = static_cast<int>(FromVarUInt(tempInStream));
            if (0 < size)
            {
                uint8 kwt = tempInStream.ReadByte();
                uint8 vwt = tempInStream.ReadByte();

                const Type& typeKey = type.params[1];
                const Type& typeValue = type.params[0];

                // 类型检测
                const std::string& kst = typeKey.type;
                if (! MSeriaHelper::CheckTypeWT(kst, (EWireType)kwt))
                {
                    LDC_LOG_ERR("wt not match! " << (int)kwt << ", " << kst);
                    return false;
                }
                const std::string& vst = typeValue.type;
                if (! MSeriaHelper::CheckTypeWT(vst, (EWireType)vwt))
                {
                    LDC_LOG_ERR("wt not match! " << (int)vwt << ", " << vst);
                    return false;
                }

                for (int i = 1; i <= size; ++i)
                {
                    if (! DecodeItem(-1, L, tempInStream, typeKey, syntax))
                    {
                        LDC_LOG_ERR("decode item err!" << typeKey.type);
                        return false;
                    }

                    if (! DecodeItem(-1, L, tempInStream, typeValue, syntax))
                    {
                        LDC_LOG_ERR("decode item err!" << typeValue.type);
                        return false;
                    }

                    lua_settable(L, -3);
                }
            }
        }
        inStream.Skip(len);
        return true;
    }

    static inline bool LoadStream(lua_State* L, NetInStream& inStream,
        const char* classname, const SSyntax& syntax, bool isRaw = false, bool isReadLen = true, bool isMem = false, bool isFirst = false)
    {
        LDC_LOG_DEBUG("LoadStream classname:"<<classname<<", offSet = "<<inStream.GetOffset()<<", left = "<<inStream.BytesLeft());

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

        try
        {
            if (! LoadCompound(L, inStream, message, syntax, isRaw, isReadLen, isMem, isFirst))
            {
                MSeriaHelper::ProcessErrFunc(L);

                LDC_LOG_ERR("LoadCompound err!"<<message.m_name);
                lua_pushnil(L);
                lua_pushfstring(L, "ERROR: LoadCompound %s", message.m_name.c_str());
                return false;
            }
        }
        catch (stream_error& e)
        {
            MSeriaHelper::ProcessErrFunc(L);

            LDC_LOG_ERR("LoadItemFromStream " << message.m_name << " exception: " << e.code);
            lua_pushnil(L);
            lua_pushfstring(L, "ERROR: LoadCompound Expection %d", e.code);
            return false;
        }

        lua_pushboolean(L, 1);
        lua_pushboolean(L, 1);
        return true;
    }

    static inline bool LoadProtobufCompound(lua_State* L, NetInStream& inStream,
        const SMessage& smessage, const SSyntax& syntax)
    {
        LDC_LOG_DEBUG("LoadProtobufCompound offSet = "<<inStream.GetOffset()<<", left ="<<inStream.BytesLeft());

        if (! MSeriaHelper::NewObject(L, smessage.m_name.c_str()) ) // 创建新table
        {
            LDC_LOG_ERR("MSeriaHelper::NewObject fail, "<<smessage.m_name);
            return false;
        }

        lua_getfield(L, -1, STR_UNSERIA);
        if (lua_isnil(L, -1)) // 描述式
        {
            LDC_LOG_DEBUG("Compound desc， "<<smessage.m_name);
            lua_pop(L, 1); // del nil

            return LoadCompoundByDesc(L, inStream, smessage, syntax, true, false, false);
        }

        // 自定义式
        if (! lua_isfunction(L, -1))
        {
            LDC_LOG_ERR("no UnSerialize func, class="<<smessage.m_name);
            return false;
        }

        LDC_LOG_DEBUG("Compound self, classname = "<<smessage.m_name);

        lua_pushvalue(L, -2); // push new obj
        lua_pushlightuserdata(L, (void*)&inStream); // ins
        lua_call(L, 2, 0);
        
        return true;
    }

    static inline bool LoadProtobufAny(int fsIndex, lua_State* L, NetInStream& inStream,
        const SSyntax& syntax)
    {
        UNUSED(fsIndex);   

        LDC_LOG_DEBUG("LoadProtobufAny offSet = "<<inStream.GetOffset());

        // 跳过4个字节的总长度, 注意运行到此处已经解析过2字节的索引, 所以从总长度开始
        inStream.Skip(4);

        // 读标识

        uint32 idx = (uint32) FromVarUInt(inStream);
        LDC_LOG_DEBUG("ref read varint idx = "<<idx);

        auto ref_ret = syntax.m_ref_id.find(idx);
        if (syntax.m_ref_id.end() == ref_ret) // 没注册这个类名的映射
        {
            LDC_LOG_ERR("not register ref id map, id = "<<idx);
            return false;
        }

        const std::string& classname = ref_ret->second->first;
        LDC_LOG_DEBUG("ref find classname = "<<classname<<", id = "<<idx);

        auto iter = syntax.m_messages.find(classname);
        if (syntax.m_messages.end() == iter)
        {
            LDC_LOG_ERR("not class," << classname);
            return false;
        }

        const SMessage& smessage = iter->second;
        LDC_LOG_DEBUG(smessage.m_name);

        // 读值
        return LoadProtobufCompound(L, inStream, smessage, syntax);
    }

    static inline bool DecodeBaseBySelfDef(lua_State* L, NetInStream& inStream, 
        const char* baseName, bool& isProcSelfDef)
    {
        LDC_LOG_DEBUG("DecodeBaseBySelfDef offSet = "<<inStream.GetOffset()<<", left = "<<inStream.BytesLeft());

        if (! MSeriaHelper::GetTableFromG(L, baseName) )
        {
            LDC_LOG_ERR("MSeriaHelper::GetTableFromG fail, "<<baseName);
            return false;
        }

        if (MSeriaHelper::CheckLuaFuncExist(L, -1, STR_UNSERIA))
        {
            isProcSelfDef = true;

            lua_getfield(L, -1, STR_UNSERIA);
            lua_pushvalue(L, -3);
            lua_pushlightuserdata(L, (void*)&inStream);
            lua_call(L, 2, 0);
            lua_pop(L, 1); // del base obj
        }
        else
        {
            isProcSelfDef = false;
            lua_pop(L, 1); // del base obj
        }
        
        return true;
    }

    static inline bool LoadProtobufReference(int fsIndex, lua_State* L, NetInStream& inStream,
        const SSyntax& syntax, const std::string& baseName)
    {
        LDC_LOG_DEBUG("LoadProtobufReference baseName = "<<baseName);

        // 序列化方保证写入时校验
        if (! LoadProtobufAny(fsIndex, L, inStream, syntax))
        {
            LDC_LOG_ERR("get Reference obj err");
            return false;
        }

        lua_getfield(L, -1, STR_CLASSNAME);
        if (! lua_isstring(L, -1))
        {
            LDC_LOG_ERR("classname not string,"<<lua_type(L, -1));
            lua_pop(L, 1);
            return false;
        }

        const std::string sonName(lua_tostring(L,-1));
        LDC_LOG_DEBUG("LoadProtobufReference sonName = "<<sonName);
        lua_pop(L, 1);
        if (! MSeriaHelper::CheckInheritance(syntax, sonName, baseName))
        {
            LDC_LOG_ERR("inheritance not exist,"<<sonName<<"，"<<baseName);
            lua_pop(L, 1); // 把刚才解析出来的obj删掉
            lua_pushnil(L);
            return false;
        }

        return true;
    }

};

}