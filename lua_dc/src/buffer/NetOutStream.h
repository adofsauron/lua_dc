#pragma once

#include "ByteStream.h"
#include "NetBuffer.h"

namespace _LUA_DC
{

class NetBuffer;

class NetOutStream
{
public:
	NetOutStream(COStream& os) 
		: m_os(os)
	{
	}

	inline void Write(const void* p, uint32 n)
	{
		m_os.Write(p, n);
	}

	inline void WriteShort(int16 s)
	{
		m_os << s;
	}

	inline void WriteUShort(uint16 us)
	{
		m_os << us;
	}

	inline void WriteInt(int i)
	{
		m_os << i;
	}

	inline void WriteUInt(uint32 i)
	{
		m_os << i;
	}

	inline void WriteLong(int64 i)
	{
		m_os << i;
	}

	inline void WriteULong(uint64 i)
	{
		m_os << i;
	}

	inline void WriteByte(uint8 b)
	{
		m_os << b;
	}

	inline void WriteSByte(char c)
	{
		m_os << c;
	}

	inline void WriteFloat(float f)
	{
		m_os << f;
	}

	inline void WriteDouble(double d)
	{
		m_os << d;
	}

	inline void WriteString(const char * s, bool wlen=false)
	{
		int l = (int) strlen(s);
		if (wlen) // 写len
		{
			m_os << (l + 1);
		}
		m_os.Write(s, l); // 写buff
		m_os << '\0';
	}

	inline void WriteBuffer(NetBuffer* buffer)
	{
		m_os << buffer->Buffer();
	}

	inline int GetBufferLength()
	{
		return m_os.GetBuf()->Size();
	}

	inline void* GetBufferBytes()
	{
		return m_os.GetBuf()->GetBytes();
	}

	inline int GetOffset()
	{
		return m_os.Pos();
	}

	inline void Seek(int offset)
	{
		m_os.Seek(offset);
	}

	inline void Skip(int n)
	{
		m_os.Skip(n);
	}

private:
	COStream& m_os;
};

};