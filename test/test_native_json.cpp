#include <boost/test/unit_test.hpp>
#include "native_json.h"
#include <boost/lexical_cast.hpp>

using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_types)
{
	json_value val(123LL);
	BOOST_REQUIRE(val.as_int() == 123LL);
}
