#include "engine.h"
#include "leveldb/db.h"

using namespace sofadb;
using namespace leveldb;

DbEngine::DbEngine(const std::string &&filename)
{
	Options opts;
	opts.block_size=1024;
	opts.create_if_missing = true;
	opts.paranoid_checks=1;
	opts.compression=kSnappyCompression;

	DB *db;
	leveldb::Status status = leveldb::DB::Open(opts,
											   filename.c_str(), &db);
	this->keystore_.reset(db);
}

DbEngine::~DbEngine()
{

}
