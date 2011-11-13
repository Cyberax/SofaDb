#ifndef BINARY_STREAM_HPP
#define BINARY_STREAM_HPP

#include "common.h"
#include <netinet/in.h>
#include <vector>
#include <boost/cast.hpp>

namespace utils {

	class output_stream
	{
		void write_byte_0(uint8_t ch)
		{
			write(&ch, 1);
		}
		void write_int2_0(uint16_t val)
		{
			uint16_t msb = htons(val);
			write(&msb, 2);
		}
		void write_int4_0(uint32_t val)
		{
			uint32_t msb = htonl(val);
			write(&msb, 4);
		}

	public:
		virtual ~output_stream() {};
		virtual void write(const void *data, size_t bytes)=0;

		template<class T> void write_byte(T ch)
		{
			write_byte_0(boost::numeric_cast<uint8_t>(ch));
		}

		template<class T> void write_uint2(T ch)
		{
			write_int2_0(boost::numeric_cast<uint16_t>(ch));
		}

		template<class T> void write_uint4(T ch)
		{
			write_int4_0(boost::numeric_cast<uint32_t>(ch));
		}

		template<class T> void write_sint4(T ch)
		{
			write_int4_0(boost::numeric_cast<int32_t>(ch));
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
