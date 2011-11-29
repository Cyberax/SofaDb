PROJECT(tools)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${libsofadb_SOURCE_DIR})
INCLUDE_DIRECTORIES(${LevelDb_INCLUDE})

ADD_EXECUTABLE(dumpldb dumpldb.cpp)
TARGET_LINK_LIBRARIES(dumpldb leveldb pthread)

ADD_EXECUTABLE(loadtags loadtags.cpp)
TARGET_LINK_LIBRARIES(loadtags leveldb pthread libsofadb)
