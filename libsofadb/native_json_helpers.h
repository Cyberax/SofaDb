#ifndef NATIVE_JSON_HELPERS_H
#define NATIVE_JSON_HELPERS_H

#include <string>
#include <istream>
#include "rapidjson/rapidjson.h"

namespace utils {

	class StringWriteStream
	{
		std::string &buf_;
	public:
		typedef char Ch;	//!< Character type. Only support char.

		StringWriteStream(std::string &buf) : buf_(buf)
		{
		}

		void Put(char c)
		{
			buf_.push_back(c);
		}

		void PutN(char c, size_t n)
		{
			buf_.append(n, c);
		}

		void Flush()
		{
		}
	};

	class StdStreamReadStream
	{
	public:
		typedef char Ch;	//!< Character type. Only support char.
		StdStreamReadStream(std::istream &str) : str_(str)
		{
			rdbuf_ = str.rdbuf();
		}
		~StdStreamReadStream()
		{
		}

		const static int eof = std::char_traits<char>::eof();

		char Peek() const
		{
			int res=rdbuf_->sgetc();
			if (res!=eof)
				return res;
			throw std::bad_exception();
		}
		char Take()
		{
			int res=rdbuf_->sgetc();
			if (res==eof)
				throw std::bad_exception();

			int bumped=rdbuf_->sbumpc();
			if (bumped==eof)
				read_more();
			return res;
		}

		void read_more()
		{
			str_.sync();
		}

		size_t Tell() const
		{
			return str_.tellg();
		}

		// Not implemented
		void Put(char c) { RAPIDJSON_ASSERT(false); }
		void Flush() { RAPIDJSON_ASSERT(false); }
		char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
		size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }

	private:
		std::istream &str_;
		std::basic_streambuf<char> *rdbuf_;
	};

	class BufReadStream
	{
	public:
		typedef char Ch;	//!< Character type. Only support char.

		BufReadStream(char* buffer, size_t bufferSize) :
			buffer_(buffer),
			bufferLast_(buffer+bufferSize), current_(buffer_),
			eof_(false)
		{
		}

		char Peek() const { return *current_; }
		char Take() { char c = *current_; Read(); return c; }
		size_t Tell() const { return (current_ - buffer_); }

		// Not implemented
		void Put(char c) { RAPIDJSON_ASSERT(false); }
		void Flush() { RAPIDJSON_ASSERT(false); }
		char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
		size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }

	private:
		void Read()
		{
			if (current_ < bufferLast_)
				++current_;
			else
				eof_ = true;
		}
		char *buffer_;
		char *bufferLast_;
		char *current_;
		bool eof_;
	};

}; //namespace utils

namespace rapidjson {
	//! Implement specialized version of PutN() with memset() for better performance.
	template<>
	inline void PutN(utils::StringWriteStream& stream, char c, size_t n) {
		stream.PutN(c, n);
	}
}; //namespace rapidjson

#endif //NATIVE_JSON_HELPERS_H
