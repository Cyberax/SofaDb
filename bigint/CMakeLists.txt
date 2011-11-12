PROJECT(bigint)

SET(BIGINT_SRCS
	BigIntegerAlgorithms.cc
	BigIntegerLibrary.hh
	BigUnsigned.hh
	BigIntegerAlgorithms.hh
	BigIntegerUtils.cc
	BigUnsignedInABase.cc
	BigInteger.cc
	BigIntegerUtils.hh
	BigUnsignedInABase.hh
	NumberlikeArray.hh
	BigInteger.hh
	BigUnsigned.cc
)

FIND_PACKAGE(Boost 1.42 REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

ADD_DEFINITIONS(-DBIGINT_EXPORTS)

ADD_LIBRARY(bigint STATIC ${BIGINT_SRCS})

ADD_EXECUTABLE(test_bigint testsuite.cc)
TARGET_LINK_LIBRARIES(test_bigint bigint ${Boost_LIBRARIES})
ADD_TEST(BigintTests ${CMAKE_CURRENT_BINARY_DIR}/test_bigint)
