#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE "SofaDB Tests"
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "scope_guard.h"

#include <glog/logging.h>

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
#ifdef DEBUG
	google::LogToStderr();
#endif
	ON_BLOCK_EXIT(&google::ShutdownGoogleLogging);

	return ::boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}
