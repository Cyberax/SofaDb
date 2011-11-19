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

	buf_input_stream inp;
	inp.buffer=out.buffer;
	erl_type_t in=binary_to_term(&inp);
	BOOST_REQUIRE(deep_eq(in, erl));

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

BOOST_AUTO_TEST_CASE(test_list_proper)
{
	list_ptr_t lst(new list_t());
	lst->val_=BigInteger(123);

	list_ptr_t lst2(new list_t());
	lst2->val_="Helo";
	lst->next_=lst2;

	list_ptr_t lst3(new list_t());
	lst3->val_=erl_nil_t();
	lst2->next_=lst3;

	//rp(term_to_binary([123, "Helo"])).
	CHECK(test(lst),
		  buf_t({131,108,0,0,0,2,97,123,107,0,4,72,101,108,111,106}));
}

BOOST_AUTO_TEST_CASE(test_list_improper)
{
	list_ptr_t lst(new list_t());
	atom_t atom={"a"};
	lst->val_=atom;

	list_ptr_t lst2(new list_t());
	atom_t atom2={"b"};
	lst2->val_=atom2;
	lst->next_=lst2;

	//rp(term_to_binary([a | b])).
	CHECK(test(lst),
		  buf_t({131,108,0,0,0,1,100,0,1,97,100,0,1,98}));
}

BOOST_AUTO_TEST_CASE(test_tuple_small)
{
	tuple_ptr_t t(new tuple_t());
	for(int f=0;f<255;++f)
		t->elements_.push_back(erl_type_t(BigInteger('c')));
	//rp(term_to_binary(list_to_tuple(string:copies("c", 255)))).
	unsigned const char res[]
			={131,104,255,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,
			  97,99,97,99,97,99,97,99,97,99};
	CHECK(test(t), buf_t(res, res+sizeof(res)));
}

BOOST_AUTO_TEST_CASE(test_tuple_big)
{
	tuple_ptr_t t(new tuple_t());
	for(int f=0;f<256;++f)
		t->elements_.push_back(erl_type_t(BigInteger('c')));
	//rp(term_to_binary(list_to_tuple(string:copies("c", 256)))).
	unsigned const char res[]
			={131,105,0,0,1,0,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,99,97,
			  99,97,99,97,99,97,99,97,99,97,99,97,99};
	CHECK(test(t), buf_t(res, res+sizeof(res)));
}

BOOST_AUTO_TEST_CASE(test_empty_doc)
{
	list_ptr_t lst(new list_t());
	atom_t atom={"false"};
	lst->val_=atom;

	list_ptr_t lst2(new list_t());
	lst2->val_=BigInteger(0);
	lst->next_=lst2;

	list_ptr_t lst3(new list_t());
	lst3->val_=BigInteger(0);
	lst2->next_=lst3;

	tuple_ptr_t tpl(new tuple_t());
	tpl->elements_.push_back(erl_nil_t());

	list_ptr_t lst4(new list_t());
	lst4->val_=tpl;
	lst3->next_=lst4;

	list_ptr_t lst5(new list_t());
	lst5->val_=erl_nil_t();
	lst4->next_=lst5;

	list_ptr_t lst6(new list_t());
	lst6->val_=erl_nil_t();
	lst5->next_=lst6;

	//rp(term_to_binary([false, 0, 0, {[]}, []])).
	CHECK(test(lst),
		  buf_t({131,108,0,0,0,5,100,0,5,102,97,108,115,101,97,0,97,0,
				104,1,106,106,106}));
}

BOOST_AUTO_TEST_CASE(test_binary)
{
	//rp(term_to_binary(<<"Hello">>)).
	binary_ptr_t bin=binary_t::make_from_string("Hello");
	CHECK(test(bin),
		  buf_t({131,109,0,0,0,5,72,101,108,108,111}));
}

static erl_type_t roundtrip(const std::string &js)
{
	erl_type_t res=parse_json(js);
	erl_type_t res2=parse_json(json_to_string(res));
	BOOST_REQUIRE(deep_eq(res, res2));
	return res2;
}

BOOST_AUTO_TEST_CASE(test_parse_json)
{
	roundtrip("{\"H\" : \"w\", \"this\" : [\"is\", {\"a\" : \"test\"}]}");

	roundtrip("{\"Hello\" : {\"a\" : null}}");
	roundtrip("{\"Hello\" : {\"a\" : [{}]}}");
	roundtrip("{\"Hello\" : [23123.123000]}");
	roundtrip("{\"Hello\" : [233452340523409580923485092348523309850234950923450]}");

//	erl_type_t res=roundtrip("{\"Hello\" : {}}");
//	std::cout << json_to_string(res) << std::endl;
	roundtrip("{\"Hello\" : \"world\", \"this\" : [\"is\",\"good\"], "
			   "\"tst\" : [\"a\", {\"1\" : \"2\"}], "
			   "\"val\" : true, "
			   "\"larg\" : 233452340523409580923485092348523309850234950923450"
			   "}");
}
