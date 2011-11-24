#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "engine.h"
#include "database.h"
using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_database_creation)
{
	jstring_t templ("/tmp/sofa_XXXXXX");
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
	jstring_t templ("/tmp/sofa_XXXXXX");
	mkdtemp(&templ[0]);
	DbEngine engine(templ, true);

	database_ptr ptr=engine.create_a_database("test");

	json_value js=string_to_json("{\"Hello\" : \"world\"}");
	char buf[32];
	for(int f=0;f<100; ++f)
	{
		sprintf(buf, "Hello%d", f);
		revision_t rev=ptr->put(buf, maybe_string_t(),
								  json_value(submap_d), js, true);
	}

//	revision_ptr rev=ptr->put("Hello", maybe_string_t(),
//							  json_value(submap_d),
//							  std::move(js), true);
	//BOOST_REQUIRE(rev->rev_.get().rev_ == "2028f9ff8e5094cfc9e4eb8bcca19e83");

//	revision_ptr rev2=ptr->get("Hello");
//	revision_ptr rev3=ptr->get("Hello", rev->rev_.get().to_string());

//	BOOST_REQUIRE_EQUAL(rev->json_body_, rev2->json_body_);
//	BOOST_REQUIRE_EQUAL(rev->json_body_, rev3->json_body_);
}

BOOST_AUTO_TEST_CASE(test_database_update)
{
//	std::string templ("/tmp/sofa_XXXXXX");
//	mkdtemp(&templ[0]);
//	DbEngine engine(templ, true);

//	database_ptr ptr=engine.create_a_database("test");
//	json_value js=string_to_json("{\"Hello\" : \"world\"}");
//	json_value js2=string_to_json("{\"Hello the second\" : \"world\"}");

//	revision_ptr rev=ptr->put("Hello", maybe_string_t(),
//							  json_value(submap_d), json_value(js), true);

//	revision_ptr rev2=ptr->put("Hello", maybe_string_t(),
//							   json_value(submap_d), json_value(js2), true);
//	BOOST_REQUIRE(!rev2);

//	revision_ptr rev3=ptr->put("Hello", maybe_string_t("2-Nope"),
//							   json_value(submap_d), json_value(js2), true);
//	BOOST_REQUIRE(!rev3);

//	revision_ptr rev4=ptr->put("Hello", rev->get_rev(),
//							   json_value(submap_d), json_value(js2), true);
//	BOOST_REQUIRE(rev4);
}
