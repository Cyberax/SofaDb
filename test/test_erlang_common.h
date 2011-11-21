#include "erlang_compat.h"

#define CHECK(stream, etalon) \
{ \
	if (stream.buffer != etalon) \
		BOOST_FAIL("Failure"); \
}

inline utils::buf_stream test(const erlang::erl_type_t & erl)
{
	utils::buf_stream out;
	erlang::term_to_binary(erl, &out);

	utils::buf_input_stream inp;
	inp.buffer=out.buffer;
	erlang::erl_type_t in=erlang::binary_to_term(&inp);
	BOOST_REQUIRE(erlang::deep_eq(in, erl));

	return out;
}
