PROJECT(test_sofadb)

FILE(GLOB test_sofadb_SRCS
	test_main.cpp
	test_native_json.cpp
	test_sofadb.cpp
	test_utils.cpp
)

FILE(GLOB test_sofadb_INCLUDES
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${libsofadb_SOURCE_DIR})
INCLUDE_DIRECTORIES(${bigint_SOURCE_DIR})
INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})

ADD_EXECUTABLE(test_sofadb ${test_sofadb_SRCS} ${test_sofadb_INCLUDES})
TARGET_LINK_LIBRARIES(test_sofadb libsofadb ${Boost_LIBRARIES} glog)
ADD_TEST(SofaTests ${CMAKE_CURRENT_BINARY_DIR}/test_sofadb)
