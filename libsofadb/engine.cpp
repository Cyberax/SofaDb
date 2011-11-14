#include "engine.h"
#include "leveldb/db.h"
#include <openssl/md5.h>
#include <boost/array.hpp>

using namespace sofadb;
using namespace leveldb;

void calculate_hash(std::string str, boost::array<uint8_t, 16> &res)
{
	assert(res.size()==MD5_DIGEST_LENGTH);
	MD5(reinterpret_cast<const unsigned char*>(str.data()), str.size(),
		res.data());
}

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
