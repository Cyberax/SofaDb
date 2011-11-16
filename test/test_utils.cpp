#include <boost/test/unit_test.hpp>

#include "errors.h"
using namespace sofadb;

result_code_t checkOk()
{
	return result_code_t();
}

result_code_t phail()
{
	return result_code_t(result_code_t::sWrongRevision);
}

void throwing()
{
	phail() | die_t();
}

result_code_t nonthrowing()
{
	TRYIT(throwing());
	return sok;
}

BOOST_AUTO_TEST_CASE(test_error_func)
{
	try
	{
		err(result_code_t::sWrongRevision) << "Hello!";
	} catch (const sofa_exception &ex)
	{
		BOOST_CHECK_EQUAL(std::string(ex.what()),
					std::string("Error code: 400, description: Hello!"));
	}

	err(result_code_t::sOk) << "Hello!"; //No exceptions
	checkOk() | die_t();

	try
	{
		phail() | die_t();
	} catch (const sofa_exception &ex)
	{
		BOOST_CHECK_EQUAL(std::string(ex.what()),
					std::string("Error code: 400, description: None"));
	}

	BOOST_REQUIRE_EQUAL(nonthrowing().code(), 400);
}
