PROJECT(libsofadb)

FILE(GLOB libsofadb_SRCS
	conflict.cpp
	database.cpp
	engine.cpp
	errors.cpp
	native_json.cpp
)

FILE(GLOB libsofadb_INCLUDES
	binary_stream.hpp
	common.h
	conflict.h
	database.h
	engine.h
	errors.h
	json_stream.h
	native_json.h
	native_json_helpers.h
	scope_guard.h
	storage_t.h
	vector_map.h

	rapidjson/internal/stack.h
	rapidjson/internal/strfunc.h
	rapidjson/internal/pow10.h
	rapidjson/stringbuffer.h
	rapidjson/reader.h
	rapidjson/filewritestream.h
	rapidjson/rapidjson.h
	rapidjson/document.h
	rapidjson/writer.h
	rapidjson/encodings.h
	rapidjson/encodedstream.h
	rapidjson/prettywriter.h
	rapidjson/filereadstream.h
	rapidjson/filestream.h
)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${LevelDb_INCLUDE})
INCLUDE_DIRECTORIES(${bigint_SOURCE_DIR})

ADD_DEFINITIONS(-DLIBSOFADB_EXPORTS) #-DRAPIDJSON_SSE42

ADD_LIBRARY(libsofadb SHARED ${libsofadb_SRCS} ${libsofadb_INCLUDES})
SET_TARGET_PROPERTIES(libsofadb PROPERTIES OUTPUT_NAME "sofadb")

TARGET_LINK_LIBRARIES(libsofadb crypto glog)
# ${LevelDb_LIBRARY}
#	json_spirit)# bigint)

TARGET_LINK_LIBRARIES(libsofadb -Wl,-whole-archive
	${LevelDb_LIBRARY} -Wl,-no-whole-archive)

# We must not re-export static libraries
SET_TARGET_PROPERTIES(libsofadb PROPERTIES LINK_INTERFACE_LIBRARIES "pthread")
