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

		StdStreamReadStream(std::istream &str,
							char* buffer, size_t bufferSize) :
			str_(str), buffer_(buffer), bufferSize_(bufferSize),
			bufferLast_(0), current_(buffer_),
			readCount_(0), count_(0), eof_(false)
		{
			Read();
		}
		~StdStreamReadStream()
		{
			UnFill();
		}

		char Peek() const { return *current_; }
		char Take() { char c = *current_; Read(); return c; }
		size_t Tell() const { return count_ + (current_ - buffer_); }

		void UnFill()
		{
			if (current_ < bufferLast_)
			{
				str_.unget();
				++current_;
			}
		}

		// Not implemented
		void Put(char c) { RAPIDJSON_ASSERT(false); }
		void Flush() { RAPIDJSON_ASSERT(false); }
		char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
		size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }

	private:
		void Read() {
			if (current_ < bufferLast_)
				++current_;
			else
				FillBuffer();
		}

		void FillBuffer() {
			if (!eof_) {
				count_ += readCount_;
				str_.read(buffer_, bufferSize_);
				readCount_ = str_.gcount();
				bufferLast_ = buffer_ + readCount_ - 1;
				current_ = buffer_;

				if (readCount_ < bufferSize_) {
					buffer_[readCount_] = '\0';
					eof_ = true;
					++bufferLast_;
				}
			}
		}

		std::istream &str_;
		char *buffer_;
		size_t bufferSize_;
		char *bufferLast_;
		char *current_;
		size_t readCount_;
		size_t count_;	//!< Number of characters read
		bool eof_;
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
