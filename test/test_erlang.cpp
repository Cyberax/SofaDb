#include <boost/test/unit_test.hpp>

#include "erlang_compat.h"

#include "BigIntegerUtils.hh"
#include <iostream>
using namespace erlang;
using namespace utils;
typedef std::vector<unsigned char> buf_t;

#define CHECK(stream, etalon) \
{ \
	if (stream.buffer != etalon) \
		BOOST_FAIL("Failure"); \
}

buf_stream test(const erl_type_t & erl)
{
	buf_stream out;
	term_to_binary(erl, &out);
	return out;
}

BOOST_AUTO_TEST_CASE(test_nil)
{
	//rp(term_to_binary([])).
	CHECK(test(erl_nil_t()), buf_t({131,106}));
}

BOOST_AUTO_TEST_CASE(test_atom)
{
	atom_t atom;
	atom.name_="test";
	//rp(term_to_binary(test)).
	CHECK(test(atom), buf_t({131,100,0,4,116,101,115,116}));
}

BOOST_AUTO_TEST_CASE(test_long_atom)
{
	atom_t atom;
	atom.name_=std::string(256, 'a');
	//list_to_atom(string:copies("a", 256)).
	//** exception error: a system limit has been reached
	BOOST_REQUIRE_THROW(test(atom), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(test_double)
{
	//rp(term_to_binary(1.123)).
	CHECK(test(1.123d),
		  buf_t({131,99,49,46,49,50,50,57,57,57,57,57,57,57,57,57,57,57,
				57,57,57,56,50,50,101,43,48,48,0,0,0,0,0}));
}

BOOST_AUTO_TEST_CASE(test_int_small)
{
	//rp(term_to_binary(255)).
	CHECK(test(BigInteger(255)), buf_t({131,97,255}));
}

BOOST_AUTO_TEST_CASE(test_int_medium)
{
	//rp(term_to_binary(-1)).
	CHECK(test(BigInteger(-1)), buf_t({131,98,255,255,255,255}));
	//rp(term_to_binary(134217727)).
	CHECK(test(BigInteger(134217727)), buf_t({131,98,7,255,255,255}));
	//rp(term_to_binary(-134217728)).
	CHECK(test(BigInteger(-134217728)), buf_t({131,98,248,0,0,0}));
}

BigInteger bigpow(const BigInteger &num, int e)
{
	BigInteger res=1;
	for(int f=0;f<e;++f)
		res*=num;
	return res;
}

BOOST_AUTO_TEST_CASE(test_int_big)
{
	/*
	-module(m).
	-export([pow/2]).
	-spec pow(integer(), non_neg_integer()) -> integer()
		; (float(), non_neg_integer()) -> float().
	pow(X, N) when is_integer(N), N >= 0 -> pow(X, N, 1).
	pow(_, 0, P) -> P;
	pow(X, N, A) -> pow(X, N-1, A*X).
	*/
	//c(m).
	//rp(term_to_binary(m:pow(256,255)-1)).
	BigInteger med=bigpow(256, 255)-1;
	CHECK(test(med),
		  buf_t({131,110,255,0,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255}));
	//c(m).
	//rp(term_to_binary(-m:pow(256,255)+1)).
	CHECK(test(-med),
		  buf_t({131,110,255,1,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255,255,255,255,255,255,255,255,
				255,255,255,255,255,255,255}));

	//c(m).
	//rp(term_to_binary(m:pow(256,255))).
	CHECK(test(med+1),
		  buf_t({131,111,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,1}));

	//c(m).
	//rp(term_to_binary(-m:pow(256,255))).
	CHECK(test(-med-1),
		  buf_t({131,111,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,1}));
}

BOOST_AUTO_TEST_CASE(test_string)
{
	std::string s(65534, 'c');
	buf_t etalon={131,107,255,254};
	etalon.insert(etalon.end(), 65534, 99);

	//term_to_binary(string:copies("c", 65534)).
	CHECK(test(s), etalon);
}

BOOST_AUTO_TEST_CASE(test_string_big)
{
	std::string s(65535, 'c');
	buf_t etalon={131,108,0,0,255,255};
	for(int f=0;f<65535;++f)
	{
		etalon.push_back(97);
		etalon.push_back(99);
	}
	etalon.push_back(106);

	//term_to_binary(string:copies("c", 65535)).
	CHECK(test(s), etalon);
}
