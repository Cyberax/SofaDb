PROJECT(libsofadb)

FILE(GLOB libsofadb_SRCS
	engine.cpp
	erlang_compat.cpp
	vlint.cpp
)

FILE(GLOB libsofadb_INCLUDES
	binary_stream.hpp
	common.h
	engine.h
	erlang_compat.h
	vlint.h
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${LevelDb_INCLUDE})
INCLUDE_DIRECTORIES(${json_spirit_SOURCE_DIR})

ADD_DEFINITIONS(-DLIBSOFADB_EXPORTS)

ADD_LIBRARY(libsofadb SHARED ${libsofadb_SRCS} ${libsofadb_INCLUDES})
SET_TARGET_PROPERTIES(libsofadb PROPERTIES OUTPUT_NAME "sofadb")

TARGET_LINK_LIBRARIES(libsofadb ${LevelDb_LIBRARY} ${json_spirit_library})

#TARGET_LINK_LIBRARIES(libsofadb -Wl,-whole-archive
#	${LevelDb_LIBRARY} -Wl,-no-whole-archive)

# We must not re-export static libraries
SET_TARGET_PROPERTIES(libsofadb PROPERTIES LINK_INTERFACE_LIBRARIES "pthread")
