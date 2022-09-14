#pragma once

#include "ByteStream.h"

namespace _LUA_DC
{

class NetBuffer
{
public:
	NetBuffer()
	{
	}

	NetBuffer(void* buffer, int len)
	{
		m_buffer.Put(buffer, 0, len);
	}

public:
	inline void SetLength(int size)
	{
		m_buffer.ExpandTo(size);
	}

	inline int GetLength()
	{
		return m_buffer.Size();
	}

	inline void* GetBuffer()
	{
		return m_buffer.GetBytes();
	}

	inline CDynamicStreamBuf& Buffer()
	{
		return m_buffer;
	}

private:
	CDynamicStreamBuf m_buffer;
};

};