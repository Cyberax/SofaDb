#include <boost/test/unit_test.hpp>

#include "engine.h"
using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_database_creation)
{
	std::string templ("/tmp/sofa_XXXXXX");
	mkdtemp(&templ[0]);

	DatabaseInfo info;
	{
		DbEngine engine(std::move(templ));
		info=engine.create_a_database("test");
		DatabaseInfo info2=engine.create_a_database("test");
		BOOST_REQUIRE(info == info2);
	}

	{
		DbEngine engine(std::move(templ));
		DatabaseInfo info2=engine.create_a_database("test");
		BOOST_REQUIRE(info == info2);
	}
}
