PROJECT(example)

FILE(GLOB example_SRCS
	example.cpp
)

FILE(GLOB example_INCLUDES
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${libsofadb_SOURCE_DIR})

ADD_EXECUTABLE(example ${example_SRCS} ${example_INCLUDES})
TARGET_LINK_LIBRARIES(example libsofadb)

