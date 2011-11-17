#include "engine.h"
#include "leveldb/db.h"
#include "json_spirit_utils.h"
#include <openssl/md5.h>
#include <boost/array.hpp>
#include <time.h>
#include "erlang_compat.h"
#include "errors.h"

#include "ezlogger/ezlogger_headers.hpp"

using namespace sofadb;
using namespace leveldb;
using namespace erlang;
using namespace json_spirit;

const std::string DbEngine::SYSTEM_DB("_sys");
const std::string DbEngine::DATA_DB("_data");

void calculate_hash(std::string str, boost::array<uint8_t, 16> &res)
{
	assert(res.size()==MD5_DIGEST_LENGTH);
	MD5(reinterpret_cast<const unsigned char*>(str.data()), str.size(),
		res.data());
}

DbEngine::DbEngine(const std::string &filename, bool temporary)
{
	this->filename_ = filename;
	this->temporary_ = temporary;

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
	this->keystore_.reset();
	if (temporary_)
		DestroyDB(filename_, Options());
}

Database::Database(DbEngine *parent,
		 time_t created_on, const std::string &name)
	: parent_(parent), created_on_(created_on), name_(name), closed_(false)
{
}

Database::Database(DbEngine *parent,
				   const json_spirit::Object &json)
{
	closed_=false;
	parent_=parent;
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
		database_ptr res(new Database(this, val.get_obj()));
		databases_[name]=res;
		return res;
	} else
	{
		WriteOptions w;

		database_ptr res(new Database(this, time(NULL), name));

		std::string jval(json_spirit::write_formatted(res->to_json()));
		keystore_->Put(w, db_info, jval);

		databases_[name]=res;
		return res;
	}
}

void DbEngine::checkpoint()
{
}

void Database::check_closed()
{
	guard_t g(mutex_);
	if (closed_)
		err(result_code_t::sError) << "Database " << name_ << " is closed";
}

revision_ptr Database::put(const std::string &id, const maybe_string_t& rev,
						   const std::string &doc_str, bool batched)
{
	guard_t p(parent_->mutex_);
	guard_t g(mutex_);
	check_closed();

	Value doc;
	read_or_throw(doc_str, doc);

	//Ok. That gets interesting!
	//Let's roll!
	Value val=find_value(doc.get_obj(), "_id");
	if (!val.is_null() && val.get_str()!=id)
		err(result_code_t::sError) << "Two different IDs specified";

	std::string docpath=DbEngine::DATA_DB+"/"+name_+
			DbEngine::DB_SEPARATOR+id;
	//parent_->keystore_->Get
	EZLOGGERSTREAM << "Reading " << docpath << std::endl;
}

