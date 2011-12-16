PROJECT(sofa_server)

FILE(GLOB sofa_server_SRCS
	server_main.cpp
)

FILE(GLOB sofa_server_INCLUDES
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${libsofadb_SOURCE_DIR})
INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})

ADD_EXECUTABLE(sofa_server ${sofa_server_SRCS} ${sofa_server_INCLUDES})
TARGET_LINK_LIBRARIES(sofa_server libsofadb ${Boost_LIBRARIES} glog gflags boost_system)
