#ifndef ERLANG_COMPAT_H
#define ERLANG_COMPAT_H

#include "common.h"

#include <boost/type_traits.hpp>
#include <boost/variant.hpp>
#include <boost/shared_array.hpp>

#include "vlint.h"
#include "binary_stream.hpp"

#define VERSION_MAGIC 131

#define NIL_EXT (106)
#define ATOM_EXT (100)
#define FLOAT_EXT (99)
#define STRING_EXT (107)

#define MAX_STRING_LEN 0xffff
#define LIST_EXT (108)

#define SMALL_TUPLE_EXT (104)
#define LARGE_TUPLE_EXT (105)

#define SMALL_INTEGER_EXT (97)
#define INTEGER_EXT (98)
#define SMALL_BIG_EXT (110)
#define LARGE_BIG_EXT (111)

namespace erlang {

	struct atom_t
	{
		std::string name_;
	};

	struct integer_t
	{
		std::string digits_;
	};

	struct erl_nil_t {};
	struct list_t;
	typedef boost::shared_ptr<list_t> list_ptr_t;

	struct tuple_t;
	typedef boost::shared_ptr<tuple_t> tuple_ptr_t;

	typedef boost::variant<
		erl_nil_t,
		atom_t,
		double,
		vlint,
		std::string,
		list_ptr_t,
		tuple_ptr_t
		> erl_type_t;

	struct list_t
	{
		list_ptr_t next_;
		erl_type_t val_;

		SOFADB_PUBLIC static list_ptr_t make();
		SOFADB_PUBLIC static list_ptr_t make(const list_t &other);
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
	SOFADB_PUBLIC void term_to_binary(const erl_type_t &term, utils::output_stream *out);

}; //namespace erlang

#endif // ERLANG_COMPAT_H
