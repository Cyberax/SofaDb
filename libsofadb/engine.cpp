#include "engine.h"
#include "database.h"

#include "leveldb/db.h"
#include <openssl/md5.h>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include "errors.h"

#include <iostream>
using namespace sofadb;
using namespace leveldb;

DbEngine::DbEngine(const jstring_t &filename, bool temporary)
{
	this->filename_ = filename;
	this->temporary_ = temporary;

	Options opts;
	opts.block_size=1024;
	opts.create_if_missing = true;
	opts.paranoid_checks=1;
	opts.compression=kSnappyCompression;

	DB *db;
	leveldb::Status status = leveldb::DB::Open(opts, filename, &db);
	this->keystore_.reset(db);
}

DbEngine::~DbEngine()
{
	this->keystore_.reset();
	if (temporary_)
		DestroyDB(filename_, Options());
}

void DbEngine::check(const leveldb::Status &status)
{
	if (status.ok())
		return;
	err(result_code_t::sError) << status.ToString();
}

database_ptr DbEngine::create_a_database(const jstring_t &name)
{
	guard_t g(mutex_);

	auto pos=databases_.find(name);
	if (pos!=databases_.end())
		return pos->second;

	ReadOptions opts;
	jstring_t db_info=jstring_t(SD_SYSTEM_DB)+"/"+name+DB_SEPARATOR+"dbinfo";

	std::string out;
	if (keystore_->Get(opts, db_info, &out).ok())
	{
		database_ptr res(new Database(string_to_json(out)));
		databases_[name]=res;
		return res;
	} else
	{
		WriteOptions w;
		database_ptr res(new Database(name));
		databases_[name]=res;
		keystore_->Put(w, db_info,
					   json_to_string(res->get_meta()));
		return res;
	}
}

void DbEngine::checkpoint()
{
}
