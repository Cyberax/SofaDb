PROJECT(sofa_server)

FILE(GLOB sofa_server_SRCS
	server_main.cpp
	server_common.cpp
)

FILE(GLOB sofa_server_INCLUDES
	server_common.h
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${libsofadb_SOURCE_DIR})
INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})

ADD_EXECUTABLE(sofa_server ${sofa_server_SRCS} ${sofa_server_INCLUDES})
TARGET_LINK_LIBRARIES(sofa_server libsofadb ${Boost_LIBRARIES}
	glog gflags boost_system boost_filesystem)

FILE(GLOB sofa_client_SRCS
	client_main.cpp
	server_common.cpp
)
FILE(GLOB sofa_client_INCLUDES
	server_common.h
)
ADD_EXECUTABLE(sofa_client ${sofa_client_SRCS} ${sofa_client_INCLUDES})
TARGET_LINK_LIBRARIES(sofa_client libsofadb ${Boost_LIBRARIES}
	glog gflags boost_system boost_filesystem)
