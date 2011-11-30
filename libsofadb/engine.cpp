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

class db_storage_t : public storage_t
{
	WriteOptions wo_;
	ReadOptions ro_;
	leveldb::db_ptr_t db_;
public:
	db_storage_t(leveldb::db_ptr_t db, bool sync) : db_(db)
	{
		wo_.sync = sync;
		ro_.verify_checksums = false;
	}

	virtual bool try_get(const jstring_t &key, jstring_t *res,
						 snapshot_t *snap)
	{
		assert(!snap);

		Status st = db_->Get(ro_, key, res);
		if (st.IsNotFound())
			return false;
		if (!st.ok())
			DbEngine::check(st);
		return true;
	}

	virtual void put(const jstring_t &key, const jstring_t &val)
	{
		DbEngine::check(db_->Put(wo_, key, val));
	}

	virtual snapshot_t* snapshot()
	{
		throw std::out_of_range("No snapshots allowed");
	}

	virtual void release_snapshot(snapshot_t*)
	{
		throw std::out_of_range("No snapshots allowed");
	}
};

DbEngine::DbEngine(const jstring_t &filename, bool temporary)
{
	this->filename_ = filename;
	this->temporary_ = temporary;

	Options opts;
	opts.create_if_missing = true;
	opts.paranoid_checks=0;
	opts.block_size=1024;
	opts.compression=kSnappyCompression;
	opts.write_buffer_size = 24*1024*1024;

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

storage_ptr_t DbEngine::create_storage(bool sync)
{
	return storage_ptr_t(new db_storage_t(keystore_, sync));
}
