#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include "engine.h"
#include "database.h"
using namespace sofadb;

BOOST_AUTO_TEST_CASE(test_database_creation)
{
	jstring_t templ("/tmp/sofa_XXXXXX");
	if (!mkdtemp(&templ[0]))
		throw std::bad_exception();

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
	if (!mkdtemp(&templ[0]))
		throw std::bad_exception();
	DbEngine engine(templ, true);

	database_ptr ptr=engine.create_a_database("test");

	json_value js=string_to_json("{\"Hello\" : \"world\"}");

	std::string id = "Hello";

	revision_num_t old;
	storage_ptr_t stg=engine.create_storage(false);
	//storage_ptr_t store = engine.create_batch_storage();
	for(int f=0;f<10; ++f)
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
}

BOOST_AUTO_TEST_CASE(test_database_update)
{
	std::string templ("/tmp/sofa_XXXXXX");
	if (!mkdtemp(&templ[0]))
		throw std::bad_exception();
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

BOOST_AUTO_TEST_CASE(test_revlog)
{
	jstring_t templ("/tmp/sofa_XXXXXX");
	if (!mkdtemp(&templ[0]))
		throw std::bad_exception();
	DbEngine engine(templ, true);
	database_ptr ptr=engine.create_a_database("test");

	json_value js=string_to_json("{\"Hello\" : \"world\"}");
	std::string id = "Hello";

	revision_num_t old(213, "NotExists");
	storage_ptr_t stg=engine.create_storage(false);
	//storage_ptr_t store = engine.create_batch_storage();
	for(int f=0;f<10; ++f)
	{
		revision_t rev=ptr->put(stg.get(), id, old, json_value(), js);
		old = std::move(rev.rev_);
	}

	json_value log;
	ptr->get(stg.get(), id, 0, 0, 0, &log);

	BOOST_REQUIRE_EQUAL(revlog_wrapper(log).top_rev_id(), old);

	const sublist_t &first=log.as_sublist().at(1).get_sublist();
	BOOST_REQUIRE_EQUAL(first.at(0).get_int(), 213);
	BOOST_REQUIRE_EQUAL(first.at(1).get_str(), "NotExists");
	BOOST_REQUIRE_EQUAL(first.at(2).get_bool(), false);
}

BOOST_AUTO_TEST_CASE(test_bench)
{
	jstring_t templ("/tmp/sofa_XXXXXX");
	if (!mkdtemp(&templ[0]))
		throw std::bad_exception();
	DbEngine engine(templ, true);

	database_ptr ptr=engine.create_a_database("test");

	json_value js=string_to_json("{\"Hello\" : \"world\"}");

	std::string id = "Hello";

	revision_num_t old;
	storage_ptr_t stg=engine.create_storage(false);
	for(int f=0;f<200000; ++f)
	{
		revision_t rev=ptr->put(stg.get(),
								id+int_to_string(f),
								revision_num_t(), json_value(), js);
	}
}
