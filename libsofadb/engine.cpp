#include "engine.h"
#include "leveldb/db.h"
#include "json_spirit_utils.h"
#include <openssl/md5.h>
#include <boost/array.hpp>
#include <time.h>
#include "erlang_compat.h"

using namespace sofadb;
using namespace leveldb;
using namespace erlang;

const std::string DbEngine::SYSTEM_DB("_sys");

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

Database::Database(const json_spirit::Object &json)
{
	name_=json_spirit::get(json, "name").get_str();
	created_on_=json_spirit::get(json, "created_on").get_int();
}

json_spirit::Object Database::to_json() const
{
	json_spirit::Object	res;
	res.push_back(json_spirit::Pair("name", name_));
	res.push_back(json_spirit::Pair("created_on",
									json_spirit::Value((int)created_on_)));
	return res;
}

database_ptr DbEngine::create_a_database(const std::string &name)
{
	guard_t g(mutex_);

	auto pos=databases_.find(name);
	if (pos!=databases_.end())
		return pos->second;

	ReadOptions opts;
	std::string db_info=SYSTEM_DB+"/"+name+DB_SEPARATOR+"dbinfo";

	std::string out;
	if (keystore_->Get(opts, db_info, &out).ok())
	{
		json_spirit::Value val;
		json_spirit::read_or_throw(out, val);
		database_ptr res(new Database(val.get_obj()));
		databases_[name]=res;
		return res;
	} else
	{
		WriteOptions w;

		database_ptr res(new Database(time(NULL), name));

		std::string jval(json_spirit::write_formatted(res->to_json()));
		keystore_->Put(w, db_info, jval);

		databases_[name]=res;
		return res;
	}
}

void DbEngine::checkpoint()
{
}
