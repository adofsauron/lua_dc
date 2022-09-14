#pragma once

#include "../seria.h"
#include "../def.h"

namespace _LUA_DC
{

class CRC32
{
public:
	static uint32 crcTable[];

	static inline int GetCRC32(void * buffer, int offset, int length)
	{
		char * bytes = (char *)buffer;
		uint32 crc = 0xFFFFFFFF;
		for (int i = 0; i < length; i++)
		{
			crc = ((crc >> 8) & 0x00FFFFFF) ^ crcTable[(crc ^ bytes[offset + i]) & 0xFF];
		}
		uint32 temp = crc ^ 0xFFFFFFFF;
		int t = (int)temp;
		return (t);
	}

	static inline int UpdateCRC32(uint8 c, int crc)
	{
		uint32 CRC = (uint32)crc;
		uint32 t = crcTable[(CRC ^ c) & 0xff] ^ ((CRC) >> 8);
		return (int)t;
	}
};

};