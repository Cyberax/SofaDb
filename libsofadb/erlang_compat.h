#ifndef ERLANG_COMPAT_H
#define ERLANG_COMPAT_H

#include "common.h"
#include <boost/variant.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "BigInteger.hh"
#include "binary_stream.hpp"

#define VERSION_MAGIC 131

#define NIL_EXT (106)
#define ATOM_EXT (100)
#define FLOAT_EXT (99)
#define STRING_EXT (107)
#define MAX_STRING_LEN (0xffff-1)
#define LIST_EXT (108)

#define SMALL_TUPLE_EXT (104)
#define LARGE_TUPLE_EXT (105)

#define BINARY_EXT (109)

#define SMALL_INTEGER_EXT (97)
#define INTEGER_EXT (98)
#define ERLANG_INT_MIN (-134217728)
#define ERLANG_INT_MAX (134217727)
#define SMALL_BIG_EXT (110)
#define LARGE_BIG_EXT (111)

namespace erlang {

	struct atom_t
	{
		SOFADB_PUBLIC static const atom_t TRUE;
		SOFADB_PUBLIC static const atom_t FALSE;
		std::string name_;
	};

	struct string_opt_t
	{
		std::string str_;
	};

	struct list_t;
	typedef boost::shared_ptr<list_t> list_ptr_t;

	struct tuple_t;
	typedef boost::shared_ptr<tuple_t> tuple_ptr_t;

	struct binary_t;
	typedef boost::shared_ptr<binary_t> binary_ptr_t;

	typedef boost::variant<
		boost::blank,
		atom_t,
		double,
		BigInteger,
		string_opt_t,
		list_ptr_t,
		tuple_ptr_t,
		binary_ptr_t
		> erl_type_t;

	struct list_t : public boost::enable_shared_from_this<list_t>
	{
		erl_type_t tail_;
		erl_type_t val_;

		SOFADB_PUBLIC static list_ptr_t make();

		friend class nil_list_creator;
	};

	SOFADB_PUBLIC extern const boost::blank erl_nil;

	struct binary_t
	{
		std::vector<unsigned char> binary_;

		SOFADB_PUBLIC static binary_ptr_t make();
		SOFADB_PUBLIC static binary_ptr_t make_from_string(
			const std::string &str);
		SOFADB_PUBLIC static binary_ptr_t make_from_buf(
			const unsigned char *ptr, size_t sz);
	};
	SOFADB_PUBLIC std::string binary_to_string(const erl_type_t &tp);

	struct tuple_t
	{
		std::vector<erl_type_t> elements_;

		SOFADB_PUBLIC static tuple_ptr_t make();
	};

	inline bool is_nil(const erl_type_t &t)
	{
		return t.type() == typeid(boost::blank);
	}

	inline bool is_list(const erl_type_t &t)
	{
		return t.type() == typeid(list_ptr_t);
	}
	inline list_ptr_t try_list(const erl_type_t &t)
	{
		if (!is_list(t))
			return list_ptr_t();
		return boost::get<list_ptr_t>(t);
	}
	inline list_ptr_t get_list(const erl_type_t &t)
	{
		return boost::get<list_ptr_t>(t);
	}

	inline bool advance(list_ptr_t *head)
	{
		if (!is_list((*head)->tail_))
			return false;
		(*head)=get_list((*head)->tail_);
		return true;
	}

	SOFADB_PUBLIC erl_type_t deep_copy(const erl_type_t &t);
	SOFADB_PUBLIC bool deep_eq (const erl_type_t &l, const erl_type_t &r);
	inline bool deep_ne(const erl_type_t &l, const erl_type_t &r)
	{
		return !deep_eq(l, r);
	}

	//Implements Erlang's term_to_binary functionality. Its
	//format is specified here:
	// http://www.erlang.org/doc/apps/erts/erl_ext_dist.html
	//and is implemented in otp_src/erts/emulator/beam/external.c
	//in the OTP source.
	SOFADB_PUBLIC void term_to_binary(const erl_type_t &term,
									  utils::output_stream *out);
	SOFADB_PUBLIC erl_type_t binary_to_term(utils::input_stream *in);

	SOFADB_PUBLIC std::ostream& operator <<(std::ostream& str,
											const erl_type_t &t);

}; //namespace erlang

#endif // ERLANG_COMPAT_H
