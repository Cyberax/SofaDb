# Find libleveldb.a - key/value storage system

# Dependencies
find_path(LevelDb_INCLUDE_DIR NAMES leveldb/db.h)
find_library(LevelDb_LIBRARY NAMES libleveldb.a libleveldb.lib)
