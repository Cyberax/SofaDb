PROJECT(libsofadb)

FILE(GLOB libsofadb_SRCS
	conflict.cpp
	database.cpp
	engine.cpp
	erlang_compat.cpp
	erlang_json.cpp
	errors.cpp
)

FILE(GLOB libsofadb_INCLUDES
	binary_stream.hpp
	common.h
	conflict.h
	database.h
	engine.h
	erlang_compat.h
	erlang_json.h
	errors.h
	scope_guard.h
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${LevelDb_INCLUDE})
INCLUDE_DIRECTORIES(${bigint_SOURCE_DIR})

ADD_DEFINITIONS(-DLIBSOFADB_EXPORTS)

ADD_LIBRARY(libsofadb SHARED ${libsofadb_SRCS} ${libsofadb_INCLUDES})
SET_TARGET_PROPERTIES(libsofadb PROPERTIES OUTPUT_NAME "sofadb")

TARGET_LINK_LIBRARIES(libsofadb crypto yajl glog)
# ${LevelDb_LIBRARY}
#	json_spirit)# bigint)

TARGET_LINK_LIBRARIES(libsofadb -Wl,-whole-archive
	${LevelDb_LIBRARY} bigint -Wl,-no-whole-archive)

# We must not re-export static libraries
SET_TARGET_PROPERTIES(libsofadb PROPERTIES LINK_INTERFACE_LIBRARIES "pthread")
