PROJECT(libsofadb)

FILE(GLOB libsofadb_SRCS
	engine.cpp
	erlang_compat.cpp
	erlang_json.cpp
	errors.cpp
)

FILE(GLOB libsofadb_INCLUDES
	binary_stream.hpp
	common.h
	engine.h
	erlang_compat.h
	errors.h

	ezlogger/ezlogger_format_policy.hpp
	ezlogger/ezlogger.hpp
	ezlogger/ezlogger_misc.hpp
	ezlogger/ezlogger_verbosity_level_policy.hpp
	ezlogger/ezlogger_headers.hpp
	ezlogger/ezlogger_macros.hpp
	ezlogger/ezlogger_output_policy.hpp
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${LevelDb_INCLUDE})
INCLUDE_DIRECTORIES(${json_spirit_SOURCE_DIR})
INCLUDE_DIRECTORIES(${bigint_SOURCE_DIR})

ADD_DEFINITIONS(-DLIBSOFADB_EXPORTS)

ADD_LIBRARY(libsofadb SHARED ${libsofadb_SRCS} ${libsofadb_INCLUDES})
SET_TARGET_PROPERTIES(libsofadb PROPERTIES OUTPUT_NAME "sofadb")

TARGET_LINK_LIBRARIES(libsofadb crypto yajl)
# ${LevelDb_LIBRARY}
#	json_spirit)# bigint)

TARGET_LINK_LIBRARIES(libsofadb -Wl,-whole-archive
	${LevelDb_LIBRARY} json_spirit bigint -Wl,-no-whole-archive)

# We must not re-export static libraries
SET_TARGET_PROPERTIES(libsofadb PROPERTIES LINK_INTERFACE_LIBRARIES "pthread")
