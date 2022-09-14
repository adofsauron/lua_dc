#pragma once

#include "buffer/CRC32.h"

namespace _LUA_DC
{

class MEncrypt
{
public:

    static void Encrypt(uint32 key, char bytes[], int offset, int length)
	{
		int end = length - length % 4;
		int pos = offset;
		for (; pos < offset + end; pos += 4)
		{
			bytes[pos + 0] ^= (char)(key);
			bytes[pos + 1] ^= (char)(key >> 8);
			bytes[pos + 2] ^= (char)(key >> 16);
			bytes[pos + 3] ^= (char)(key >> 24);

			key = (uint32)CRC32::GetCRC32(bytes, pos, 4);
		}
		if (pos < offset + length) bytes[pos++] ^= (char)(key);

		if (pos < offset + length) bytes[pos++] ^= (char)(key >> 8);
		if (pos < offset + length) bytes[pos++] ^= (char)(key >> 16);
	}

    static void Decrypt(uint32 key, char bytes[], int offset, int length)
	{
		int end = length - length % 4;
		int pos = offset;

		for (; pos < offset + end; pos += 4)
		{
			uint32 newkey = (uint32)CRC32::GetCRC32(bytes, pos, 4);
			bytes[pos + 0] ^= (char)(key);
			bytes[pos + 1] ^= (char)(key >> 8);
			bytes[pos + 2] ^= (char)(key >> 16);
			bytes[pos + 3] ^= (char)(key >> 24);

			key = newkey;
		}
		if (pos < offset + length) bytes[pos++] ^= (char)(key);
		if (pos < offset + length) bytes[pos++] ^= (char)(key >> 8);
		if (pos < offset + length) bytes[pos++] ^= (char)(key >> 16);
	}

    static int  GetCRC32(char bytes[], int offset, int length)
	{
		return CRC32::GetCRC32(bytes, offset, length);
	}
};

}