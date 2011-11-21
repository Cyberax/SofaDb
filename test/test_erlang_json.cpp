#include <boost/test/unit_test.hpp>

#include "test_erlang_common.h"
#include "erlang_json.h"

#include <iostream>
using namespace erlang;
using namespace utils;
typedef std::vector<unsigned char> buf_t;

static erl_type_t roundtrip(const std::string &js)
{
	erl_type_t res=parse_json(js);
	erl_type_t res2=parse_json(json_to_string(res));
	BOOST_REQUIRE(deep_eq(res, res2));
	return res2;
}

//BOOST_AUTO_TEST_CASE(test_parse_json)
//{
//	roundtrip("{\"H\" : \"w\", \"this\" : [\"is\", {\"a\" : \"test\"}]}");

//	roundtrip("{\"Hello\" : {\"a\" : null}}");
//	roundtrip("{\"Hello\" : {\"a\" : [{}]}}");
//	roundtrip("{\"Hello\" : [23123.123000]}");
//	roundtrip("{\"Hello\" : [233452340523409580923485092348523309850234950923450]}");

//	roundtrip("{\"Hello\" : \"world\", \"this\" : [\"is\",\"good\"], "
//			   "\"tst\" : [\"a\", {\"1\" : \"2\"}], "
//			   "\"val\" : true, "
//			   "\"larg\" : 233452340523409580923485092348523309850234950923450"
//			   "}");
//}

BOOST_AUTO_TEST_CASE(test_parse_json2)
{
	erl_type_t res=parse_json("{\"Hello\" : \"world\"}");

	//rp(term_to_binary([{<<"Hello">>, <<"world">>}])).
	CHECK(test(res),
		  buf_t({131,108,0,0,0,1,104,2,109,0,0,0,5,72,101,108,108,111,
				109,0,0,0,5,119,111,114,108,100,106}));
}

//BOOST_AUTO_TEST_CASE(test_programmatic_json)
//{
//	erl_type_t root=create_submap();
//	put_val(root, "Hello")=binary_t::make_from_string("world");
//	put_val(root, "test") = erl_nil_t;

//	erl_type_t sub = create_submap();
//	put_val(root, "sub") = sub;
//	put_val(sub, "a") = BigInteger(1234214);

//	erl_type_t js=parse_json("{"
//							 "\"sub\" : {\"a\" : 1234214}, "
//							 "\"test\": null,"
//							 "\"Hello\": \"world\""
//							 "}");
//	BOOST_REQUIRE(deep_eq(js, root));
//}
