#include <boost/test/unit_test.hpp>
#include "native_json.h"
#include <boost/lexical_cast.hpp>
#include "BigIntegerUtils.hh"

using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_types)
{
	json_value val(int64_t(123));
	BOOST_REQUIRE(val.as_int() == 123LL);
	val.as_int() = int64_t(1234);
	BOOST_REQUIRE(val.as_int() == 1234);
}

static json_value roundtrip(const jstring_t &js)
{
	json_value res=string_to_json(js);
	json_value res2=string_to_json(json_to_string(res));
	BOOST_REQUIRE_EQUAL(res, res2);
	return res2;
}

BOOST_AUTO_TEST_CASE(test_native)
{
	roundtrip("{\"Hello\" : \"world\"}");

	roundtrip("{\"H\" : \"w\", \"this\" : [\"is\", {\"a\" : \"test\"}]}");

	roundtrip("{\"Hello\" : {\"a\" : null}}");
	roundtrip("{\"Hello\" : {\"a\" : [{}]}}");
	roundtrip("{\"Hello\" : [23123.123000]}");
	roundtrip("{\"Hello\" : [233452340523409580923485092348523309850234950923450]}");

	roundtrip("{\"Hello\" : \"world\", \"this\" : [\"is\",\"good\"], "
			   "\"tst\" : [\"a\", {\"1\" : \"2\"}], "
			   "\"val\" : true, "
			   "\"larg\" : 233452340523409580923485092348523309850234950923450"
			   "}");
}

BOOST_AUTO_TEST_CASE(test_native_programmatic_json)
{
	json_value root;
	root.as_submap()["Hello"].as_str()="world";
	root.as_submap()["test"].as_nil();

	root.as_submap()["sub"].as_submap()["a"]=BigInteger(1234214);

	json_value js=string_to_json("{"
							 "\"test\": null,"
							 "\"sub\" : {\"a\" : 1234214}, "
							 "\"Hello\": \"world\""
							 "}");
	BOOST_REQUIRE_EQUAL(js, root);
}

BOOST_AUTO_TEST_CASE(test_normalization)
{
	json_value l(BigInteger(1234));
	json_value r(int64_t(1234));
	BOOST_REQUIRE_EQUAL(l, r);

	json_value l1(stringToBigInteger(
					  "834181234214782364897264982673496739836741789"));
	json_value r1(int64_t(1234));
	BOOST_REQUIRE_NE(l1, r1);

}
