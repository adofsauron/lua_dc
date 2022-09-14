#pragma once

#include <string>
#include "../seria.h"
#include "../log.h"

namespace _LUA_DC
{

enum STREAM_ERROR_CODE
{
	SEC_NO_ERROR = 0,
	SEC_OUT_OF_RANGE,
	SEC_INVALID_STRING,
	SEC_FIXED_STREAM_OVERFLOW,
	SEC_INVALID_CHAR,
	SEC_CHECKSUM_ERROR,
};

struct stream_error
{
	int code;

	stream_error(int c)
		: code(c)
	{
	}
};

class CBasicStreamBuf
{
protected:
	char * m_bytes;
	uint32 m_maxlen;
	uint32 m_deflen;

public:
	CBasicStreamBuf()
		:m_bytes(0), m_maxlen(1), m_deflen(0) //maxlen不为0，是为了移位需要
	{
	}
	virtual ~CBasicStreamBuf()
	{
	}
	inline char* GetBytes()const
	{
		return m_bytes;
	}
	inline uint32 Size()const
	{
		return m_deflen;
	}
	inline uint32 Capacity()const
	{
		return m_maxlen;
	}
	inline uint32 SpaceLeft()const
	{
		return m_maxlen - m_deflen;
	}

	inline void Clear()
	{
		m_deflen=0;
	}
	inline char * GetBufRange(uint32 pos, uint32 n)
	{
		uint32 np = pos + n;

		if(pos>m_deflen || n>m_deflen || np > m_deflen)
		{
			return 0;
		}
		else
		{
			return m_bytes+pos;
		}
	}

	virtual void ReAlloc(uint32 size) = 0;
	virtual void Free() = 0;

	inline uint32 ExpandTo(uint32 size)
	{
		if(size > m_maxlen)
		{
			ReAlloc(size);
		}

		if(m_deflen < size)
			m_deflen = size;

		return size;
	}

	inline uint32 Get(void* p, uint32 pos, uint32 n)
	{
		uint32 np = pos + n;
		if(pos>m_deflen || n>m_deflen || np > m_deflen)
		{
			LDC_LOG_ERR("stream out of range in CBasicStreamBuf::Get"
				<<", pos="<< pos
				<< ", n=" << n
				<< ", np=" << np
				<< ", m_deflen=" << m_deflen
				);
			throw stream_error(SEC_OUT_OF_RANGE);
		}
		memcpy(p, m_bytes+pos, n);
		return n;
	}
	inline uint32 GetNoThrow(void* p, uint32 pos, uint32 n)
	{
		uint32 np = pos + n;
		if(pos>m_deflen || n>m_deflen || np > m_deflen)
		{
			n = m_deflen-pos;

			uint32 np2 = pos + n;
			if(n>m_deflen || np2 > m_deflen) //如果还是烂的，应该不会到这里，保险起见。
			{
				n = 0;
			}
		}
		memcpy(p, m_bytes+pos, n);
		return n;
	}
	inline uint32 Put(const void* p, uint32 pos, uint32 n)
	{
		uint32 total = pos + n;
		ExpandTo(total);
		memcpy(m_bytes+pos, p, n);
		return n;
	}

	inline void CopyFrom(const CBasicStreamBuf& src)
	{
		if(this == &src)
			return;

		Clear();
		Put(src.GetBytes(), 0, src.Size());
	}

	inline CBasicStreamBuf& operator=(const CBasicStreamBuf& buf)
	{
		CopyFrom(buf);
		return *this;
	}

	inline std::string Dump()
	{
		static std::string hex;
		hex.clear();
		//测试用代码
#ifdef DEBUG_DUMP
		char buf[3];
		for (uint32 i = 0; i < m_deflen; i++)
		{
			unsigned char c = m_bytes[i];
			sprintf(buf, "%02X", c);
			buf[2] = '\0';
			hex += buf;
		}
#endif // DEBUG_DUMP
		return hex;
	}
};

class CDynamicStreamBuf : public CBasicStreamBuf
{
public:
	explicit CDynamicStreamBuf(uint32 n=128)
	{
		ReAlloc(n);
	}
	CDynamicStreamBuf(const CDynamicStreamBuf& input)
		:CBasicStreamBuf()
	{
		ReAlloc(input.Capacity());
		Put(input.GetBytes(), 0, input.Size());
	}
	inline CDynamicStreamBuf& operator=(const CDynamicStreamBuf& buf)
	{
		CopyFrom(buf);
		return *this;
	}
	inline CDynamicStreamBuf& operator=(const CBasicStreamBuf& buf)
	{
		CopyFrom(buf);
		return *this;
	}
	inline CDynamicStreamBuf & operator=(CDynamicStreamBuf && buf)
	{
		if(this != &buf)
		{
			std::swap(m_bytes, buf.m_bytes);
			std::swap(m_deflen, buf.m_deflen);
			std::swap(m_maxlen, buf.m_maxlen);
		}
		return *this;
	}
	virtual ~CDynamicStreamBuf()
	{
		Free();
	}
	inline virtual void ReAlloc(uint32 size) override
	{
		if(size <= m_maxlen)
			return;

		do
		{
			m_maxlen <<= 1;
		}while(size > m_maxlen);
		char* nbuf = new char[m_maxlen];
		if(m_bytes)
		{
			if(Size())
			{
				memcpy(nbuf, m_bytes, Size());
			}
			delete []m_bytes;
		}
		m_bytes = nbuf;
	}
	inline virtual void Free() override
	{
		delete []m_bytes;
	}
	inline void Append(const CDynamicStreamBuf& input)
	{
		Put(input.GetBytes(), Size(), input.Size());
	}
};

class CBorrowedStreamBuf : public CBasicStreamBuf
{
public:
	CBorrowedStreamBuf(char* bytes, uint32 len)
	{
		m_bytes = bytes;
		m_maxlen = len;
		m_deflen = len;
	}
protected:
	inline virtual void ReAlloc(uint32 size) override
	{
		UNUSED(size);
		LDC_LOG_ERR("borrowed buffer overflow");
		throw stream_error(SEC_FIXED_STREAM_OVERFLOW);
	}
	inline virtual void Free() override
	{
		LDC_LOG_ERR("borrowed, no free");
	}
};


// ------------- stream classes -----------------

class CStreamBase
{
protected:
	CBasicStreamBuf* m_streambuf;
	uint32 m_pos;
public:
	CStreamBase(CBasicStreamBuf *buf)
	{
		m_streambuf = buf;
		m_pos = 0;
	}
	virtual ~CStreamBase()
	{
	}
    inline void Attach(CBasicStreamBuf *buf)
    {
        m_pos = 0;
        m_streambuf = buf;
    }
	inline uint32 Pos()
	{
		return m_pos;
	}
	inline CBasicStreamBuf* GetBuf()const
	{
		return m_streambuf;
	}
	inline uint32 Seek(uint32 n)
	{
		m_pos = n;
		return m_pos;
	}
};

struct default_endian_filter
{
	inline static const short& N2H(const short& v){return v;}
	inline static const short& H2N(const short& v){return v;}
	//------------------------------------
	inline static const unsigned short& N2H(const unsigned short& v) { return v; }
	inline static const unsigned short& H2N(const unsigned short& v) { return v; }
	//------------------------------------
	inline static const int& N2H(const int& v) { return v; }
	inline static const int& H2N(const int& v) { return v; }
	//------------------------------------
	inline static const unsigned int& N2H(const unsigned int& v) { return v; }
	inline static const unsigned int& H2N(const unsigned int& v) { return v; }
	//------------------------------------
	inline static const long& N2H(const long& v) { return v; }
	inline static const long& H2N(const long& v) { return v; }
	//------------------------------------
	inline static const unsigned long& N2H(const unsigned long& v) { return v; }
	inline static const unsigned long& H2N(const unsigned long& v) { return v; }
	//------------------------------------
	inline static const int64& N2H(const int64& v) { return v; }
	inline static const int64& H2N(const int64& v) { return v; }
	//------------------------------------
	inline static const uint64& N2H(const uint64& v) { return v; }
	inline static const uint64& H2N(const uint64& v) { return v; }
};

class CStreamDefaultEndian : public CStreamBase
{
public:
	typedef default_endian_filter TF;
	CStreamDefaultEndian(CBasicStreamBuf* buf) : CStreamBase(buf){}
	CStreamDefaultEndian(CBasicStreamBuf& buf) : CStreamBase(&buf){}
};

template<class Base>
class CIStreamGeneric : public Base
{
public:
	CIStreamGeneric(const CBasicStreamBuf* buf) : Base(const_cast<CBasicStreamBuf*>(buf)){}
	CIStreamGeneric(const CBasicStreamBuf& buf) : Base(const_cast<CBasicStreamBuf*>(&buf)){}
	~CIStreamGeneric(){}

	inline int BytesLeft()
	{
		return (int)Base::m_streambuf->Size() - (int)Base::m_pos;
	}
	inline operator bool()
	{
		return BytesLeft() > 0;
	}
	inline const char* Skip(uint32 n)
	{
		char* ret = Base::m_streambuf->GetBufRange(Base::m_pos, n);

		if(ret)
		{
			Base::m_pos += n;
		}
		else
		{
			LDC_LOG_ERR("stream out of range in Skip()");
			throw stream_error(SEC_OUT_OF_RANGE);
		}
		return ret;
	}
	inline uint32 Read(void* p, uint32 n)
	{
		uint32 res = Base::m_streambuf->Get(p, Base::m_pos, n);
		Base::m_pos += res;
		return res;
	}
	inline uint32 ReadNoThrow(void* p, uint32 n)
	{
		uint32 res = Base::m_streambuf->GetNoThrow(p, Base::m_pos, n);
		Base::m_pos += res;
		return res;
	}
	//////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(CBasicStreamBuf &buf)
	{
		unsigned int size;
		(*this)>>size;
		buf.Put(Skip(size), 0, size);
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(bool &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = v & 0x01;
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(char &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	inline CIStreamGeneric& operator >>(unsigned char &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(int &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = Base::TF::N2H(v);
		return *this;
	}
	inline CIStreamGeneric& operator >>(unsigned int &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = Base::TF::N2H(v);
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(short &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = Base::TF::N2H(v);
		return *this;
	}
	inline CIStreamGeneric& operator >>(unsigned short &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = Base::TF::N2H(v);
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(long &v)
	{
		int64 v64;
		operator >>(v64);
		v = (long)v64;
		return *this;
	}
	inline CIStreamGeneric& operator >>(unsigned long &v)
	{
		uint64 v64;
		operator >>(v64);
		v = (unsigned long)v64;
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(float &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric& operator >>(double &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric &operator >>(int64 &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = Base::TF::N2H(v);
		return *this;
	}
	///////////////////////////////////////////////////////////
	inline CIStreamGeneric &operator >>(uint64 &v)
	{
		Base::m_pos += Base::m_streambuf->Get(&v,Base::m_pos,sizeof(v));
		v = Base::TF::N2H(v);
		return *this;
	}
	///////////////////////////////////////////////////////////
	//template<typename T>
	//CIStreamGeneric &operator >>(T &v)
	//{
	//	lightAssert(0);
	//	return *this;
	//}
	///////////////////////////////////////////////////////////

	inline CIStreamGeneric& ReadString(char* &str, uint32 maxlen)
	{
		unsigned int len;
		(*this)>>len;
		if(maxlen == 0 || len>maxlen-1)
		{
			LDC_LOG_ERR("invalid string");
			throw stream_error(SEC_INVALID_STRING);
		}
		str = Base::GetBuf()->GetBytes()+Base::Pos();
		Skip(len+sizeof(char));

		if(str[len]!=0)
		{
			LDC_LOG_ERR("invalid string");
			throw stream_error(SEC_INVALID_STRING);
		}
		return *this;
	}
};

template<class Base>
class COStreamGeneric : public Base
{
public:
	COStreamGeneric(CBasicStreamBuf *buf) : Base(buf){}
	COStreamGeneric(CBasicStreamBuf &buf) : Base(&buf){}
	~COStreamGeneric(){}
	inline uint32 Write(const void* p, uint32 n)
	{
		uint32 res = Base::m_streambuf->Put(p, Base::m_pos, n);
		Base::m_pos += res;
		return res;
	}
	inline uint32 Write(CIStreamGeneric<Base> &is)
	{
		uint32 size = is.GetBuf()->Size() - is.Pos();
		return Write(is.Skip(size), size);
	}
	inline const char* Skip(uint32 n)
	{
		uint32 oldp = Base::m_pos;
		Base::m_pos += n;
		Base::m_streambuf->ExpandTo(Base::m_pos);
		return Base::m_streambuf->GetBytes() + oldp;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const CBasicStreamBuf &buf)
	{
		unsigned int size = (unsigned int)buf.Size();
		(*this)<<size;
		Write(buf.GetBytes(), size);
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const bool &v)
	{
		Base::m_pos += Base::m_streambuf->Put(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const char &v)
	{
		Base::m_pos += Base::m_streambuf->Put(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	inline COStreamGeneric& operator <<(const unsigned char &v)
	{
		Base::m_pos += Base::m_streambuf->Put(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const int &v)
	{
		int var = Base::TF::H2N(v);
		Base::m_pos += Base::m_streambuf->Put(&var,Base::m_pos,sizeof(v));
		return *this;
	}
	inline COStreamGeneric& operator <<(const unsigned int &v)
	{
		unsigned int var = Base::TF::H2N(v);
		Base::m_pos += Base::m_streambuf->Put(&var,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const short &v)
	{
		short var = Base::TF::H2N(v);
		Base::m_pos += Base::m_streambuf->Put(&var,Base::m_pos,sizeof(v));
		return *this;
	}
	inline COStreamGeneric& operator <<(const unsigned short &v)
	{
		unsigned short var = Base::TF::H2N(v);
		Base::m_pos += Base::m_streambuf->Put(&var,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const long &v)
	{
		return operator <<((const int64)v);
	}
	inline COStreamGeneric& operator <<(const unsigned long &v)
	{
		return operator <<((const uint64)v);
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const float &v)
	{
		Base::m_pos += Base::m_streambuf->Put(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric& operator <<(const double &v)
	{
		Base::m_pos += Base::m_streambuf->Put(&v,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric &operator <<(const int64 &v)
	{
		int64 var = Base::TF::H2N(v);
		Base::m_pos += Base::m_streambuf->Put(&var,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	inline COStreamGeneric &operator <<(const uint64 &v)
	{
		uint64 var = Base::TF::H2N(v);
		Base::m_pos += Base::m_streambuf->Put(&var,Base::m_pos,sizeof(v));
		return *this;
	}
	//////////////////////////////////////////////////
	//template <typename T>
	//COStreamGeneric &operator <<(const T&)
	//{
	//	lightAssert(0);
	//	return *this;
	//}
};


template<class ST>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const char* str)
{
	unsigned int len = (unsigned int)strlen(str);
	os<<len;
	os.Write(str, len);
	char eos=0;
	os<<eos;
	return os;
}
template<class ST>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, char* str)
{
	unsigned int len = (unsigned int)strlen(str);
	os<<len;
	os.Write(str, len);
	char eos=0;
	os<<eos;
	return os;
}

template<class ST>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, const char* &str)
{
	unsigned int len;
	is>>len;
	str = is.GetBuf()->GetBytes()+is.Pos();
	is.Skip(len);
	is.Skip(sizeof(char));

	if(str[len]!=0)
	{
		LDC_LOG_ERR("invalid string");
		throw stream_error(SEC_INVALID_STRING);
	}
	return is;
}
template<class ST>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, char* &str)
{
	unsigned int len;
	is>>len;
	str = is.GetBuf()->GetBytes()+is.Pos();
	is.Skip(len);
	is.Skip(sizeof(char));

	if(str[len]!=0)
	{
		LDC_LOG_ERR("invalid string");
		throw stream_error(SEC_INVALID_STRING);
	}
	return is;
}



template<class ST, int L>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, char (&str)[L])
{
	char* src;
	is.ReadString(src, L);
	safe_strcpy(str, src);//不应该用memcpy，因为存储的时候用的是strcpy
	return is;
}

template<class ST>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const std::string &str)
{
	return os<<str.c_str();
}
template<class ST>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, std::string &str)
{
	const char* s;
	is>>s;
	str = s;
	return is;
}

//////////////////////////////////////////////////////////////////////////
template<typename ST, typename T>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, T* p)
{
	os << (int64)p;
	return os;
}

template<typename ST, typename T>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, T* &p)
{
	int64 tmp;
	is >> tmp;
	p = (T*)tmp;
	return is;
}
template<typename ST, typename T, int L>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, T (&p)[L])
{
	return is;
}

#include <vector>

template<typename ST, typename T1, typename T2>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const std::pair<T1, T2> & pair)
{
	os << pair.first << pair.second;
	return os;
}
template<typename ST, typename T1, typename T2>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, std::pair<T1, T2> & pair)
{
	is >> pair.first >> pair.second;
	return is;
}

template<typename ST, typename T>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const std::vector<T> & vec)
{
	os << static_cast<int> ( vec.size() );
	for( int i=0; i<(int)vec.size(); ++i)
	{
		os << vec[i];
	}
	return os;
}
template<typename ST, typename T>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, std::vector<T> & vec)
{
	vec.clear();
	int num;
	is >> num;
	for(int i = 0; i < num; ++ i )
	{
		T val;
		is >> val;
		vec.push_back( val );
	}
	return is;
}

// #include <list>
// template<typename ST, typename T>
// inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const std::list<T> & ilist)
// {
// 	os << static_cast<int> ( ilist.size() );
// 	for(typename std::list<T>::const_iterator it = ilist.begin(); it != ilist.end(); ++ it)
// 	{
// 		os << * it;
// 	}
// 	return os;
// }
// template<typename ST, typename T>
// inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, std::list<T> & ilist)
// {
// 	ilist.clear();
// 	int num;
// 	is >> num;
// 	for(int i = 0; i < num; ++ i )
// 	{
// 		T val;
// 		is >> val;
// 		ilist.push_back( val );
// 	}
// 	return is;
// }

#include <map>
template<typename ST, typename K, typename V>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const std::map<K, V> & amap)
{
	os << static_cast<int> ( amap.size() );
	for(typename std::map<K, V>::const_iterator it = amap.begin(); it != amap.end(); ++ it)
	{
		os << it->first << it->second;
	}
	return os;
}

template<typename ST, typename K, typename V>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, std::map<K, V> & amap)
{
	amap.clear();
	int num;
	is >> num;
	for(int i = 0; i < num; ++ i )
	{
		K key;
		V val;
		is >> key >> val;
		amap.insert(std::pair<K,V>(key, val));
	}
	return is;
}

#include <set>
template<typename ST, typename V, typename LESS>
inline COStreamGeneric<ST> &operator <<(COStreamGeneric<ST> &os, const std::set<V, LESS> & aset)
{
	os << static_cast<int> ( aset.size() );
	for(typename std::set<V, LESS>::const_iterator it = aset.begin(); it != aset.end(); ++ it)
	{
		os << *it;
	}
	return os;
}

template<typename ST, typename V, typename LESS>
inline CIStreamGeneric<ST> &operator >>(CIStreamGeneric<ST> &is, std::set<V, LESS> & aset)
{
	aset.clear();
	int num;
	is >> num;
	for(int i = 0; i < num; ++ i )
	{
		V val;
		is >> val;
		aset.insert(val);
	}
	return is;
}

inline void CheckCharValid(const char* str)//checks only the validness of chars
{
	while(*str)
	{
		if((unsigned char)*str < 32)
		{
			LDC_LOG_ERR("invalid char");
			throw stream_error(SEC_INVALID_CHAR);
		}
		++str;
	}
}
inline void CheckCharValid(const std::string& str)//checks only the validness of chars
{
	CheckCharValid(str.c_str());
}

typedef CIStreamGeneric<CStreamDefaultEndian> CIStream;
typedef COStreamGeneric<CStreamDefaultEndian> COStream;

};