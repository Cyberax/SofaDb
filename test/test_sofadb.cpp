#include <boost/test/unit_test.hpp>

#include "engine.h"
using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_database_creation)
{
	std::string templ("/tmp/sofa_XXXXXX");
	mkdtemp(&templ[0]);

	database_ptr info;
	{
		DbEngine engine(templ, false);
		info=engine.create_a_database("test");
		database_ptr info2=engine.create_a_database("test");
		BOOST_REQUIRE(*info == *info2);
	}

	{
		DbEngine engine(templ, true);
		database_ptr info2=engine.create_a_database("test");
		BOOST_REQUIRE(*info == *info2);
	}
}

BOOST_AUTO_TEST_CASE(test_database_put)
{
	std::string templ("/tmp/sofa_XXXXXX");
	mkdtemp(&templ[0]);
	DbEngine engine(templ, true);

	database_ptr ptr=engine.create_a_database("test");

//	revision_ptr rev=ptr->put("hello", maybe_string_t(),
//							  "{\"Hello\" : \"world\"}", true);
}
