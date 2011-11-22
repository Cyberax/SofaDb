#include <boost/test/unit_test.hpp>
#include "erlang_json.h"
#include <boost/lexical_cast.hpp>

#include "engine.h"
using namespace sofadb;
using namespace erlang;

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

	erl_type_t js=parse_json("{\"Hello\" : \"world\"}");
//	for(int f=0;f< 100000; ++f)
//	{
//		std::string id ="Hello" + boost::lexical_cast<std::string>(f);
//		revision_ptr rev=ptr->put(id, maybe_string_t(), js, true);
//	}

	revision_ptr rev=ptr->put("Hello", maybe_string_t(), js, true);
	BOOST_REQUIRE(rev->rev_.get().rev_ == "2028f9ff8e5094cfc9e4eb8bcca19e83");

	revision_ptr rev2=ptr->get("Hello");
	revision_ptr rev3=ptr->get("Hello", rev->rev_.get().to_string());

	BOOST_REQUIRE(deep_eq(rev->json_body_, rev2->json_body_));
	BOOST_REQUIRE(deep_eq(rev->json_body_, rev3->json_body_));
}
