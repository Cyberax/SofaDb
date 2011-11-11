#include <boost/test/unit_test.hpp>

#include "erlang_compat.h"
using namespace erlang;
using namespace utils;

BOOST_AUTO_TEST_CASE(test_nil)
{
	erl_type_t nil=erl_nil_t();
	buf_stream out;

	term_to_binary(nil, &out);
	out.buffer.size();
}
