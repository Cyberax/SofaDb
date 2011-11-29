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
	for(int f=0;f<100; ++f)
	{
		revision_t rev=ptr->put(stg.get(), id, old, json_value(), js);
		old = std::move(rev.rev_);
	}
	//engine.commit_batch(store, false);
	//ptr->commit_batch(batch, false);

	json_value v1; revision_t r1;
	ptr->get(stg.get(), id, 0, &v1, &r1);
	json_value v2; revision_t r2;
	ptr->get(stg.get(), id, &old, &v2, &r2);
	json_value v3; revision_t r3;
	ptr->get(stg.get(), id, &old, &v3, &r3);

	BOOST_REQUIRE_EQUAL(r1.rev_, r2.rev_);
	BOOST_REQUIRE_EQUAL(r1.rev_, r3.rev_);

	BOOST_REQUIRE_EQUAL(js, v1);
	BOOST_REQUIRE_EQUAL(js, v2);

	for(int f=0;f<100000;++f)
	{
		json_value v3; revision_t r3;
		ptr->get(stg.get(), id, &old, &v3, &r3);
	}
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
