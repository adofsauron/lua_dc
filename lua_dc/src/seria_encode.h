#pragma once

#include "seria.h"
#include "def.h"
#include "tools.h"
#include "lua_wrap.h"
#include "seria_helper.h"
#include "encrypt.h"
#include "buffer/NetOutStream.h"
#include "buffer/NetBuffer.h"

namespace _LUA_DC
{

class MSeriaEncode
{
public:

    static bool EncodeNetMessage(lua_State* L, const SSyntax& syntax);

    static bool EncodeMemoryObject(lua_State* L, const SSyntax& syntax);

    static bool RawSerialize(lua_State* L, const SSyntax& syntax);

private:
    static bool EncodeItem(int fsIndex, lua_State* L, NetOutStream& outStream,
        const SSyntax& syntax, const Type& types);

    static bool EncodeCompoundOptionSelfDef(lua_State* L, NetOutStream& outStream,
        const SMessage& message, const SSyntax& syntax, bool byBase = false, bool WriteLen = false, bool isMem = false, bool isfirst = false);

    static inline void ToVarInt(int64 value, NetOutStream& outStream)
    {
        uint64 lvalue = static_cast<uint64>((value << 1) ^ (value >> 63));
        for (;;)
        {
            uint8 aByte = lvalue & 0x7f;
            lvalue >>= 7;
            if (0 == lvalue)
            {
                outStream.WriteByte(aByte);
                break;
            }
            else
            {
                aByte |= 0x80;
                outStream.WriteByte(aByte);
            }
        }
    }

    // 对正整数做优化
    static inline void ToVarUInt(uint64 value, NetOutStream& outStream)
    {
        if (value < 0x80)
        {
            outStream.WriteByte(static_cast<uint8>(value));
            return;
        }

        for (;;)
        {
            uint8 aByte = value & 0x7f;
            value >>= 7;
            if (0 == value)
            {
                outStream.WriteByte(aByte);
                break;
            }
            else
            {
                aByte |= 0x80;
                outStream.WriteByte(aByte);
            }
        }
    }

    static inline void SaveFieldDesc(int fsIndex, const EWireType wireType, NetOutStream& outStream)
    {
        if (-1 == fsIndex)
        {
            return;
        }

        uint16 value = (uint16) (fsIndex << 3 | wireType);
        LDC_LOG_DEBUG("fsIndex = "<<fsIndex
            <<", wireType = "<<wireType
            <<", value = "<<value
            <<", offSet = "<<outStream.GetOffset()
            );

        ToVarUInt(static_cast<uint64>(value), outStream);

    }

    // 描述式的序列化
    static inline bool EncodeCompoundByDesc(lua_State* L, NetOutStream& outStream,
        const SMessage& message, const SSyntax& syntax, bool isMem)
    {
        UNUSED(isMem);
        LDC_LOG_DEBUG("EncodeCompoundByDesc offSet = "<<outStream.GetOffset());
        for (const auto& index : message.m_keys)
        {
            const auto& iter_field = message.m_fields.find(index);
            if (message.m_fields.end() == iter_field)
            {
                LDC_LOG_ERR("no filed," << index);
                return false;
            }

            const SField& field = iter_field->second;

            LDC_LOG_DEBUG((lua_typename(L, lua_type(L, -1))));
    
            lua_getfield(L, -1, field.m_name.c_str());
            if (! lua_isnil(L, -1))
            {
                LDC_LOG_DEBUG("ToStream:" << field.m_name);
                if(! EncodeItem((int)index, L, outStream, syntax, field.m_type))
                {
                    LDC_LOG_ERR("EncodeItem,"<<index<<","<<field.m_name);
                    return false;
                }
            }
            lua_pop(L, 1);
        }

        // 开始处理基类
        if (message.m_base.empty())
        {
            return true;
        }

        auto iter = syntax.m_messages.find(message.m_base);
        if (syntax.m_messages.end() == iter)
        {
            LDC_LOG_ERR("can't find base class，this="<<message.m_name<<", base="<<message.m_base);
            return false;
        }

        const SMessage& fmes = iter->second; // 基类
        if (! MSeriaHelper::GetTableFromG(L, fmes.m_name.c_str()))
        {
            LDC_LOG_ERR("GetTableFromG fail,"<<fmes.m_name);
            return false;
        }

        // outStream.WriteUShort(DEF_INHERIT_FLAG);
        ToVarUInt(EWireType::DelBase, outStream);

        lua_getfield(L, -1, STR_SERIA);

        LDC_LOG_DEBUG("EncodeCompoundByDesc for Base offSet = "<<outStream.GetOffset());
        return EncodeCompoundOptionSelfDef(L, outStream, fmes, syntax, true, false, true, false);
    }

    static inline bool EncodeCompoundByDescWithLen(lua_State* L, NetOutStream& outStream,
        const SMessage& message, const SSyntax& syntax, bool isMem)
    {
        int lenOffset = outStream.GetOffset();
        outStream.Skip(4);

        bool ret = EncodeCompoundByDesc(L, outStream, message, syntax, isMem);

        outStream.Seek(lenOffset);
        int len = (outStream.GetBufferLength() - lenOffset - 4);
        outStream.WriteInt(len);
        outStream.Seek(outStream.GetBufferLength());
        
        LDC_LOG_DEBUG("EncodeCompoundByDescWithLen len = "<<len);
        return ret;
    }

    static inline bool SaveProtobufCompound(int fsIndex, lua_State* L,
        NetOutStream& outStream, const SMessage& message, const SSyntax& syntax, bool isRaw, bool writeLen, bool isMem, bool isFirst)
    {
        SaveFieldDesc(fsIndex, EWireType::LengthDelimited, outStream);

        LDC_LOG_DEBUG("SaveProtobufCompound offSet = "<<outStream.GetOffset());

        if (isRaw)
        {
            if (isFirst)
            {
                return EncodeCompoundByDescWithLen(L, outStream, message, syntax, isMem);
            }
            else
            {
                return EncodeCompoundByDesc(L, outStream, message, syntax, isMem);
            }
        }

        lua_getfield(L, -1, STR_SERIA);
        return EncodeCompoundOptionSelfDef(L, outStream, message, syntax, false, writeLen, isMem, isFirst);
    }

    static inline bool SaveStream(lua_State* L, NetOutStream& outStream,
        const SMessage& message, const SSyntax& syntax, bool isRaw, bool writeLen, bool isMem, bool isFirst)
    {
        return SaveProtobufCompound(-1, L, outStream, message, syntax, isRaw, writeLen, isMem, isFirst);
    }

     static inline bool ToStream(lua_State* L, NetOutStream& outStream,
        const SMessage& message, const SSyntax& syntax, bool encrypt, uint32 key)
    {
        uint32 securityFlag = DEF_SECRITY_FLAG;
        int checkSum = 0; 

        int theStart = outStream.GetOffset();

        LDC_LOG_DEBUG(securityFlag);

        outStream.WriteUInt(securityFlag);
        outStream.WriteInt(0); // checksum
        outStream.WriteInt(0); // length

        LDC_LOG_DEBUG("ToStream offSet = "<<outStream.GetOffset());

        bool ret = SaveStream(L, outStream, message, syntax, false, true, true, true);

        int theEnd = outStream.GetOffset();
        int buffStart = theStart + 12; // sizeof(uint) + sizeof(int) + sizeof(int)
        int buffLength = theEnd - buffStart;

        if (encrypt)
        {
            MEncrypt::Encrypt(key, (char*)outStream.GetBufferBytes(), buffStart, buffLength);
            checkSum = MEncrypt::GetCRC32((char*)outStream.GetBufferBytes(), buffStart, buffLength);
        }

        LDC_LOG_DEBUG(checkSum);
        LDC_LOG_DEBUG(buffLength);
        LDC_LOG_DEBUG(buffStart);

        outStream.Seek(theStart + 4); // start + secflag
        outStream.WriteInt(checkSum); // checksum
        outStream.WriteInt(buffLength); // length
        outStream.Seek(theEnd);

        return ret;
    }

    static inline void SaveType(const std::string& ft, NetOutStream& outStream)
    {
        EValueType vt = MSeriaHelper::GetValueType(ft);
        EWireType wt = MSeriaHelper::GetWireType(vt);
        outStream.WriteByte(wt);
    }

    static inline void SaveProtobuf(int fsIndex, int value, NetOutStream& outStream, EWireType wiretype)
    {
        if (wiretype == EWireType::EVarInt)
        {
            SaveFieldDesc(fsIndex, EWireType::EVarInt, outStream);
            ToVarInt(value, outStream);
        }
        else
        {
            SaveFieldDesc(fsIndex, EWireType::Data32, outStream);
            outStream.WriteInt(value);
        }
    }

    static inline void SaveProtobuf(int fsIndex, int64 value, NetOutStream& outStream, EWireType wiretype)
    {
        if (wiretype == EWireType::EVarInt)
        {
            SaveFieldDesc(fsIndex, EWireType::EVarInt, outStream);
            ToVarInt(value, outStream);
        }
        else
        {
            SaveFieldDesc(fsIndex, EWireType::Data64, outStream);
            outStream.WriteLong(value);
        }
    }

    static inline void SaveProtobuf(int fsIndex, float value, NetOutStream& outStream)
    {
        SaveFieldDesc(fsIndex, EWireType::Data32, outStream);
        outStream.WriteFloat(value);
    }

    static inline void SaveProtobuf(int fsIndex, float64 value, NetOutStream& outStream)
    {
        SaveFieldDesc(fsIndex, EWireType::Data64, outStream);
        outStream.WriteDouble(value);
    }

    static inline void SaveProtobuf(int fsIndex, const char* value, size_t len, NetOutStream& outStream)
    {
        SaveFieldDesc(fsIndex, EWireType::LengthDelVarUInt, outStream);

        ToVarUInt(static_cast<uint64>(len), outStream);
        outStream.Write(value, len);
    }

    static inline void SaveProtobuf(int fsIndex, NetBuffer& value, size_t len, NetOutStream& outStream)
    {
        SaveFieldDesc(fsIndex, EWireType::LengthDelVarUInt, outStream);

        ToVarUInt(static_cast<uint64>(len), outStream);
        outStream.Write(value.GetBuffer(), value.GetLength());
    }

    static inline bool SaveProtobufList(int fsIndex, const Type& type,
        lua_State* L, NetOutStream& outStream, const SSyntax& syntax)
    {
        int idx = 0;
        while (lua_geti(L, -1, ++idx))
        {
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        int size = idx - 1;
        SaveFieldDesc(fsIndex, EWireType::LengthDelimited, outStream);
        int lenOffset = outStream.GetOffset();
        outStream.Skip(4);
        // outStream.WriteInt((int)size);
        ToVarUInt(static_cast<uint64>(size), outStream);

        if (size > 0)
        {
            const Type& vType = type.params[0];
            SaveType(vType.type, outStream);
            
            idx = 0;
            while (lua_geti(L, -1, ++idx))
            {
                if (! EncodeItem(-1, L, outStream, syntax, vType))
                {
                    LDC_LOG_ERR("save list EncodeItem err，idx="<<idx)
                    return false;
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }

        outStream.Seek(lenOffset);
        outStream.WriteInt(outStream.GetBufferLength() - lenOffset - 4);
        outStream.Seek(outStream.GetBufferLength());
        return true;
    }

    static inline bool SaveProtobufMap(int fsIndex, const Type& type,
         lua_State* L, NetOutStream& outStream, const SSyntax& syntax)
    {
        if (type.params.size() != 2)
        {
            LDC_LOG_ERR("map params size !=2");
            return false;
        }

        int size = 0;
        lua_pushnil(L);
        while (lua_next(L, -2))
        {
            ++size;
            lua_pop(L, 1);
        }

        SaveFieldDesc(fsIndex, EWireType::LengthDelimited, outStream);
        int lenOffset = outStream.GetOffset();
        outStream.Skip(4);
        // outStream.WriteInt((int)size);
        ToVarUInt(static_cast<uint64>(size), outStream);

        const Type& typeKey = type.params[1];
        const Type& typeValue = type.params[0];

        if (size > 0)
        {
            SaveType(typeKey.type, outStream);
            SaveType(typeValue.type, outStream);

            lua_pushnil(L);
            while (lua_next(L, -2))
            {
                lua_pushvalue(L, -2); // stack...top -> key
                
                if (! EncodeItem(-1, L, outStream, syntax, typeKey))
                {
                    LDC_LOG_ERR("save map key EncodeItem err，"<<typeKey.type)
                    return false;
                }

                lua_pop(L, 1); // stack top -> value
                if(! EncodeItem(-1, L, outStream, syntax, typeValue))
                {
                    LDC_LOG_ERR("save map value EncodeItem err，"<<typeValue.type)
                    return false;
                }

                lua_pop(L, 1);
            }
        }

        outStream.Seek(lenOffset);
        outStream.WriteInt(outStream.GetBufferLength() - lenOffset - 4);
        outStream.Seek(outStream.GetBufferLength());
        return true;
    }

    static inline bool WrapSaveStream(lua_State* L, NetOutStream& outStream,
        const char* classname, const SSyntax& syntax, bool isRaw, bool writeLen, bool isMem, bool isFirst)
    {
        LDC_LOG_DEBUG("WrapSaveStream classname:"<<classname);

        auto iter = syntax.m_messages.find(classname);
        if (syntax.m_messages.end() == iter)
        {
            LDC_LOG_ERR("not class,"<<classname);
            lua_pushnil(L);
            lua_pushfstring(L, "not class, %s", classname);
            return false;
        }

        const SMessage& message = iter->second;
        LDC_LOG_DEBUG(message.m_name);

        if (! SaveStream(L, outStream, message, syntax, isRaw, writeLen, isMem, isFirst))
        {
            LDC_LOG_ERR("SaveStream fail,"<<message.m_name);
            lua_pushnil(L);
            lua_pushfstring(L, "SaveStream fail,", classname);
            return false;
        }

        int len = outStream.GetBufferLength();
        void* bytes = outStream.GetBufferBytes();

        LDC_LOG_DEBUG("ENCODE LEN:"<<len);
        lua_pushboolean(L, 1);
        lua_pushlstring(L, (const char*)bytes, len);

        return true;
    }

    static inline bool SaveProtobufAny(int fsIndex, lua_State* L,
         NetOutStream& outStream, const SSyntax& syntax)
    {
        LDC_LOG_DEBUG("SaveProtobufAny offSet = "<<outStream.GetOffset());

        lua_getfield(L, -1, STR_CLASSNAME);
        if (! lua_isstring(L, -1))
        {
            lua_pop(L, 1);
            LDC_LOG_ERR("Any obj but not has clasname");
            return false;
        }

        std::string classname(lua_tostring(L, -1));
        lua_pop(L, 1);

        auto iter = syntax.m_messages.find(classname);
        if (syntax.m_messages.end() == iter)
        {
            LDC_LOG_ERR("not class," << classname);
            return false;
        }

        const SMessage& smessage = iter->second;
        LDC_LOG_DEBUG(smessage.m_name);

        // 协议部分

        // 索引:2字节
        SaveFieldDesc(fsIndex, EWireType::LengthDelimited, outStream);

        // 总长度占位:4字节
        int lenOffset = outStream.GetOffset();
        outStream.Skip(4); // any的整个字节流的长度

        // 写标识
        auto ref_ret = syntax.m_ref_name.find(classname);
        if (syntax.m_ref_name.end() == ref_ret) // 没注册这个类名的映射
        {
            LDC_LOG_ERR("not register ref map, classname = "<<classname);
            return false;
        }
        
        uint32 idx = ref_ret->second;
        LDC_LOG_DEBUG("ref, classname = "<<classname<<", idx="<<idx);

        // 直接用varint写
        ToVarUInt(idx, outStream);

        LDC_LOG_DEBUG("SaveProtobufAny value offSet = "<<outStream.GetOffset());

        // 写值
        if(!SaveProtobufCompound(fsIndex, L, outStream, smessage, syntax, false, true, false, false))
        {
            LDC_LOG_ERR("SaveProtobufCompound fail," << classname);
            return false;
        }

        // 填充整个字节流的长度
        outStream.Seek(lenOffset);
        outStream.WriteInt(outStream.GetBufferLength() - lenOffset - 4);
        outStream.Seek(outStream.GetBufferLength());

        return true;
    }

    static inline bool SaveProtobufReference(int fsIndex, lua_State* L,
         NetOutStream& outStream, const SSyntax& syntax, const std::string& baseName)
    {
        LDC_LOG_DEBUG("SaveProtobufReference offSet = "<<outStream.GetOffset());

        lua_getfield(L, -1, STR_CLASSNAME);
        if (! lua_isstring(L, -1))
        {
            LDC_LOG_ERR("classname not string,"<<lua_type(L, -1));
            lua_pop(L, 1);
            return false;
        }

        const std::string sonName(lua_tostring(L,-1));
        lua_pop(L, 1);
        if (! MSeriaHelper::CheckInheritance(syntax, sonName, baseName))
        {
            LDC_LOG_ERR("inheritance not exist,"<<sonName<<"，"<<baseName);
            return false;
        }

        return SaveProtobufAny(fsIndex, L, outStream, syntax);
    }
};

}