#include <boost/test/unit_test.hpp>
#include "native_json.h"
#include <boost/lexical_cast.hpp>

using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_types)
{
	json_value val(int64_t(123));
	BOOST_REQUIRE(val.as_int() == 123LL);
	val.as_int() = int64_t(1234);
	BOOST_REQUIRE(val.as_int() == 1234);
}

BOOST_AUTO_TEST_CASE(test_native)
{
	json_value val=string_to_json("{\"Hello\" : \"world\"}");

	BOOST_REQUIRE(val.is_submap());
}
