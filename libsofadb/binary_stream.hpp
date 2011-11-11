#ifndef BINARY_STREAM_HPP
#define BINARY_STREAM_HPP

#include "common.h"
#include <netinet/in.h>
#include <vector>

namespace utils {

	class output_stream
	{
	public:
		virtual ~output_stream() {};
		virtual void write(const void *data, size_t bytes)=0;

		void write_byte(unsigned char ch)
		{
			write(&ch, 1);
		}

		void write_int2(unsigned short val)
		{
			unsigned short msb = htons(val);
			write(&msb, 2);
		}

		void write_int4(unsigned int val)
		{
			unsigned int msb = htonl(val);
			write(&msb, 4);
		}
	};

	class buf_stream : public output_stream
	{
	public:
		std::vector<unsigned char> buffer;
		void write(const void *data, size_t bytes)
		{
			buffer.insert(buffer.end(),
						  static_cast<const char*>(data),
						  static_cast<const char*>(data)+bytes);
		}
	};

}  // namespace utils

#endif  //BINARY_STREAM_HPP
