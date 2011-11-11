#ifndef ERLANG_COMPAT_H
#define ERLANG_COMPAT_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits.hpp>
#include <boost/variant.hpp>
#include <boost/shared_array.hpp>
#include <type_traits>

#include <netinet/in.h>

#define VERSION_MAGIC 131
#define NIL_EXT (106)
#define ATOM_EXT (100)
#define FLOAT_EXT (99)
#define STRING_EXT (107)
#define MAX_STRING_LEN 0xffff
#define LIST_EXT (108)

namespace erlang {

	class output_stream
	{
	public:
		virtual ~output_stream() {};
		virtual void write(const void *data, size_t bytes);

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

	struct atom_t
	{
		std::string name_;
	};

	struct integer_t
	{
		std::string digits_;
	};

	struct nil_t {};
	struct list_t;
	typedef boost::shared_ptr<list_t> list_ptr_t;

	struct tuple_t;
	typedef boost::shared_ptr<tuple_t> tuple_ptr_t;

	typedef boost::variant<
		nil_t,
		atom_t,
		double,
		integer_t,
		std::string,
		list_ptr_t,
		tuple_ptr_t
		> erl_type_t;

	struct list_t
	{
		list_ptr_t next_;
		erl_type_t val_;

		static list_ptr_t make();
		static list_ptr_t make(const list_t &other);
	};

	struct tuple_t
	{
		std::vector<erl_type_t> elements_;
	};

	//Implements Erlang's term_to_binary functionality. Its
	//format is specified here:
	// http://www.erlang.org/doc/apps/erts/erl_ext_dist.html
	//and is implemented in otp_src/erts/emulator/beam/external.c
	//in the OTP source.
	void term_to_binary(const erl_type_t &term, output_stream *out);

}; //namespace erlang

#endif // ERLANG_COMPAT_H
