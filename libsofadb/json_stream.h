#ifndef JSON_STREAM_H
#define JSON_STREAM_H

#include "common.h"

namespace sofadb {
	class json_value;

	class json_stream
	{
	public:
		virtual ~json_stream() {}

		virtual void write_null() = 0;
		virtual void write_bool(bool val) = 0;
		virtual void write_int(int64_t i) = 0;
		virtual void write_double(double d) = 0;
		virtual void write_string(const char *str, size_t ln) = 0;
		virtual void write_digits(const char *str, size_t ln) = 0;
		virtual void start_map() = 0;
		virtual void end_map() = 0;
		virtual void start_list() = 0;
		virtual void end_list() = 0;

		void write_string(const jstring_t &str)
		{
			write_string(str.data(), str.length());
		}

		virtual void write_json(const json_value &val)=0;
	};

	class json_read_stream
	{
	public:
		virtual ~json_read_stream() {}

		virtual void read_null() = 0;
		virtual bool read_bool() = 0;
		virtual int64_t read_int() = 0;
		virtual double read_double() = 0;
		virtual jstring_t read_string() = 0;
		virtual jstring_t read_digits() = 0;

		virtual void expect_map() = 0;
		virtual void unexpect_map() = 0;
		virtual void expect_list() = 0;
		virtual void unexpect_list() = 0;
	};

	SOFADB_PUBLIC std::auto_ptr<json_stream> make_stream(
		jstring_t &append_to, bool pretty=false);

}; //namespace sofadb

#endif //JSON_STREAM_H
