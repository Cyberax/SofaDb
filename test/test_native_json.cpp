#include <boost/test/unit_test.hpp>
#include "native_json.h"
#include "errors.h"
#include <boost/lexical_cast.hpp>

using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_types)
{
	json_value val(int64_t(123));
	BOOST_REQUIRE(val.as_int() == 123LL);
	val.as_int() = int64_t(1234);
	BOOST_REQUIRE(val.as_int() == 1234);
}

BOOST_AUTO_TEST_CASE(test_grafts)
{
	json_value val(submap_d);

	json_value val2(jstring_t("world"));
	val["hello"].as_graft() = graft_t(&val2);

	json_value etalon=string_to_json("{\"hello\" : \"world\"}");
	BOOST_REQUIRE_EQUAL(val, etalon);

	json_value copy = val;
	BOOST_REQUIRE_EQUAL(val, copy);
}

BOOST_AUTO_TEST_CASE(test_big_ints)
{
	json_value n1(bignum_t("-1"));
	json_value n2(bignum_t("1"));
	BOOST_CHECK(n1!=n2);
	BOOST_CHECK(n1<n2);

	json_value n3(bignum_t("-1"));
	json_value n4(int64_t(1));
	BOOST_CHECK(n3 < n4);
	BOOST_CHECK(n3.normalize_int().get_int()==-1);

	BOOST_CHECK(bignum_t("-1") > bignum_t("-1000"));
	BOOST_CHECK(bignum_t("1") > bignum_t("-1000"));
	BOOST_CHECK(bignum_t("1000") > bignum_t("10"));
	BOOST_CHECK(bignum_t("11") > bignum_t("10"));
	BOOST_CHECK(bignum_t("-12") < bignum_t("-11"));

	json_value v1(bignum_t("9223372036854775807"));
	json_value v2(bignum_t("-9223372036854775808"));
	BOOST_CHECK(v1.normalize_int().is_int());
	BOOST_CHECK(v2.normalize_int().is_int());

	json_value l1(bignum_t("9223372036854775808"));
	json_value l2(bignum_t("-9223372036854775809"));
	BOOST_CHECK(l1.normalize_int().is_big_int());
	BOOST_CHECK(l2.normalize_int().is_big_int());
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

	root.as_submap()["sub"].as_submap()["a"]=bignum_t("1234214");

	json_value js=string_to_json("{"
							 "\"test\": null,"
							 "\"sub\" : {\"a\" : 1234214}, "
							 "\"Hello\": \"world\""
							 "}");
	BOOST_REQUIRE_EQUAL(js, root);
}

BOOST_AUTO_TEST_CASE(test_normalization)
{
	json_value l(bignum_t("1234"));
	json_value r(int64_t(1234));
	BOOST_REQUIRE_EQUAL(l, r);

	json_value l1(bignum_t("834181234214782364897264982673496739836741789"));
	json_value r1(int64_t(1234));
	BOOST_REQUIRE_NE(l1, r1);
}

BOOST_AUTO_TEST_CASE(test_json_bugz)
{
	try {
		json_value js=string_to_json("{"
							 "\"testpaddddddddddddd\": null,"
							 "\"sub\" : \"a\" : 1234214, "
							 "\"Hello\": \"world\""
							 "}");
		BOOST_FAIL("No exception");
	} catch(const sofa_exception &ex)
	{
		BOOST_REQUIRE_EQUAL(ex.err().code(), result_code_t::sWrongRevision);
	}

	try {
		json_value js=string_to_json(""
							 "\"testpaddddddddddddd\": null,"
							 "\"Hello\": \"world\""
							 "}");
		BOOST_FAIL("No exception");
	} catch(const sofa_exception &ex)
	{
		BOOST_REQUIRE_EQUAL(ex.err().code(), result_code_t::sWrongRevision);
	}
}
