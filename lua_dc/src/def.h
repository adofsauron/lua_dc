#pragma once

#include <string.h>
#include <stdint.h>
#include <float.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace _LUA_DC 
{

typedef double					float64;
//typedef char					int8;
typedef unsigned char			uint8;
typedef short					int16;
typedef unsigned short			uint16;
typedef int						int32;
typedef unsigned int			uint32;
typedef long long				int64;
typedef unsigned long long		uint64;


#define STR_OBJECT		    "Object"
#define STR_CLASSNAME	    "_className"
#define STR_SUPER		    "_protoBase"
#define STR_PROTO		    "_proto"
#define STR_SERIA           "Serialize"
#define STR_UNSERIA         "UnSerialize"

#define LOG_FILE_SELF       "luadc.log" // 当项目没有注入输出函数时,向此文件写入

// 校验码
#define DEF_SECRITY_FLAG  (0xacabdeaf)
#define DEF_INHERIT_FLAG  (0xFEFE)

#ifndef UNUSED
#define UNUSED(x) (void)x;
#endif

// wire type enum
enum EWireType
{
    LengthDelimited     = 0,
    EVarInt             = 1,
    Data32              = 2,
    Data64              = 3,
    DelBase             = 4, //基类分割
    LengthDelVarUInt    = 5, // 后边跟随的长度为varuint，TODO: 暂时性的, 随后用两次遍历清除统一都为uvarint
};

enum EValueType
{
    NotSupported    = 0,
    Int32           = 1,
    Int64           = 2,
    Float32         = 3,
    Float64         = 4,
    String          = 5,
    ENetBuffer      = 6,
    Time64          = 7,
    List            = 8,
    Map             = 9,
    Compound        = 10,
    Byte			= 11,
    Any             = 12,
    Reference       = 13,
};

enum LDC_RIDX_DEF
{
    LDC_RIDX_SYNTAX = LDC_RIDX_START,   // 语法树
    LDC_RIDX_FUNC_LOG,                  // 日志输出函数
    LDC_RIDX_FUNC_ERR,                  // 错误回调函数
    LDC_RIDX_END,
};

// 变长存储
#define USE_VAR_INT32
#define USE_VAR_INT64

// 类型描述
struct Type
{
    std::string			type;
    std::vector<Type>   params;
};

// 属性描述
struct SField
{
    SField()
        :m_index(0)
    {
    }

    std::string 	m_name;  //  字段名字
    int 			m_index; //  字段index
    Type			m_type;  //  字段类型
} ;

// 语法树
struct SMessage
{
    SMessage()
    {
    }

    std::string             m_name;
    std::set<int>			m_keys;
    std::map<int,SField> 	m_fields;
    std::string 			m_base;
};

// 语法树森林
struct SSyntax
{
    SSyntax()
    {
    }

    // 语法森林
    using TypeSyntax    = std::unordered_map<std::string, SMessage>;

    // 名字与id的映射, 双向; 注意标识使用int类型
    using TypeRefName   = std::unordered_map<std::string, uint32>;
    using TypeRefID     = std::unordered_map<uint32, TypeRefName::iterator>;

    TypeSyntax      m_messages;
    TypeRefName     m_ref_name;
    TypeRefID       m_ref_id;

};

}

