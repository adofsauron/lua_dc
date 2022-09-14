#pragma once

#include "ByteStream.h"
#include "NetBuffer.h"

namespace _LUA_DC
{

class NetBuffer;

class NetInStream
{
public:
	NetInStream(CIStream& is)
		: m_is(is)
	{
	}

	NetInStream(void* buffer, int start, int length)
		: m_is(*(new CIStream(m_buffer))) // RAII
	{
		m_buffer.Put((char *)buffer + start, 0, length);
	}

	~NetInStream()
	{
		if (m_buffer.Size() > 0)
		{
			delete &m_is; // RAII
		}
	}

private:
	NetInStream(const NetInStream&) = delete;
	NetInStream& operator=(const NetInStream&) = delete;

public:
	float inline ReadFloat()
	{
		float f;
		m_is >> f;
		return f;
	}

	double inline ReadDouble()
	{
		double d;
		m_is >> d;
		return d;
	}

	int16 inline ReadShort()
	{
		short s;
		m_is >> s;
		return s;
	}

	uint16 inline ReadUShort()
	{
		unsigned short us;
		m_is >> us;
		return us;
	}

	int inline ReadInt()
	{
		int i;
		m_is >> i;
		return i;
	}

	uint32 inline ReadUInt()
	{
		unsigned int ui;
		m_is >> ui;
		return ui;
	}

	int64 inline ReadLong()
	{
		long l;
		m_is >> l;
		return l;
	}

	uint64 inline ReadULong()
	{
		unsigned long ul;
		m_is >> ul;
		return ul;
	}

	uint8 inline ReadByte()
	{
		unsigned char uc;
		m_is >> uc;
		return uc;
	}

	char inline ReadSByte()
	{
		char c;
		m_is >> c;
		return c;
	}

	inline const char* ReadString(char* str, int len)
	{
		m_is.Read(str, len);
		return str;
	}

	inline NetBuffer* ReadBuffer(NetBuffer* netBuffer)
	{
		const int len = ReadInt();
		char* byte = m_is.GetBuf()->GetBytes();
		byte += 4;
		netBuffer->Buffer().Put(byte, 0, len);
		Skip(len);
		return netBuffer;
	}

	void inline Skip(int size)
	{
		m_is.Skip(size);
	}

	void inline Seek(int offset)
	{
		m_is.Seek(offset);
	}

	int inline BytesLeft()
	{
		return m_is.BytesLeft();
	}

	int inline GetBufferLength()
	{
		return m_is.GetBuf()->Size();
	}

	inline void*  GetBufferBytes()
	{
		return m_is.GetBuf()->GetBytes();
	}

	int inline GetOffset()
	{
		return m_is.Pos();
	}

	inline const char* Dump()
	{
		m_dump = m_is.GetBuf()->Dump();
		return m_dump.c_str();
	}


private:
	CIStream& m_is;
	CDynamicStreamBuf m_buffer;
	std::string m_dump;
};

};