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

	class input_stream
	{
	public:
		virtual ~input_stream() {};
		virtual void read(void *data, size_t bytes)=0;

		uint8_t read_byte()
		{
			uint8_t res;
			read(&res, 1);
			return res;
		}

		uint16_t read_uint2()
		{
			uint16_t res;
			read(&res, 2);
			return ntohs(res);
		}

		int16_t read_sint2()
		{
			return static_cast<int16_t>(read_uint2());
		}

		uint32_t read_uint4()
		{
			uint32_t res;
			read(&res, 4);
			return ntohl(res);
		}

		int32_t read_sint4()
		{
			return static_cast<int32_t>(read_uint4());
		}
	};

	class buf_input_stream : public input_stream
	{
		typedef std::vector<unsigned char> buf_t;
	public:
		buf_t buffer;
		buf_t::const_iterator pos;

		buf_input_stream()
		{
			pos=buf_t::const_iterator();
		}

		void read(void *data, size_t bytes)
		{
			if (pos==buf_t::const_iterator())
				pos=buffer.begin();

			if (buffer.end()-pos < bytes)
				throw std::out_of_range("Premature end of buffer");

			std::copy(pos, pos+bytes, static_cast<char*>(data));
			pos += bytes;
		}
	};


}  // namespace utils

#endif  //BINARY_STREAM_HPP
