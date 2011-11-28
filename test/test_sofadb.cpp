#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

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

	std::string id = "Hello";

	revision_num_t old;
	storage_ptr_t stg=engine.create_storage(false);
	//storage_ptr_t store = engine.create_batch_storage();
	for(int f=0;f<100000; ++f)
	{
		revision_t rev=ptr->put(stg.get(),
								id+int_to_string(f), old, json_value(), js);
		old = std::move(rev.rev_);
	}
	//engine.commit_batch(store, false);
	//ptr->commit_batch(batch, false);

	Database::res_t rev2=ptr->get(stg.get(), id);
	Database::res_t rev3=ptr->get(stg.get(), id, old);
	BOOST_REQUIRE_EQUAL(rev2.first.rev_, rev3.first.rev_);
	BOOST_REQUIRE_EQUAL(rev2.second, rev3.second);

	BOOST_REQUIRE_EQUAL(js, rev2.second);
	BOOST_REQUIRE_EQUAL(js, rev3.second);
}

BOOST_AUTO_TEST_CASE(test_database_update)
{
	std::string templ("/tmp/sofa_XXXXXX");
	mkdtemp(&templ[0]);
	DbEngine engine(templ, true);

	database_ptr ptr=engine.create_a_database("test");
	storage_ptr_t stg=engine.create_storage(false);

	json_value js=string_to_json("{\"Hello\" : \"world\"}");
	json_value js2=string_to_json("{\"Hello the second\" : \"world\"}");

	revision_t rev=ptr->put(stg.get(), "Hello", revision_num_t(),
							  json_value(submap_d), js);

	revision_t rev2=ptr->put(stg.get(), "Hello", revision_num_t(),
							   json_value(submap_d), js2);
	BOOST_REQUIRE(rev2.empty());

	revision_t rev3=ptr->put(stg.get(), "Hello", revision_num_t("2-Nope"),
							   json_value(submap_d), js2);
	BOOST_REQUIRE(rev3.empty());

	revision_t rev4=ptr->put(stg.get(), "Hello", rev.rev_,
							   json_value(submap_d), js2);
	BOOST_REQUIRE(!rev4.empty());
}
