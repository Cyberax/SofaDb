#include "engine.h"
#include "leveldb/db.h"
#include "erlang_json.h"
#include <openssl/md5.h>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include "erlang_compat.h"
#include "errors.h"

#include <iostream>
using namespace sofadb;
using namespace leveldb;
using namespace erlang;

const std::string DbEngine::SYSTEM_DB("_sys");
const std::string DbEngine::DATA_DB("_data");
const revision_info_t revision_info_t::empty_revision;

std::string sofadb::calculate_hash(const std::vector<unsigned char> &arr)
{
	static const char alphabet[17]="0123456789abcdef";

	unsigned char res[MD5_DIGEST_LENGTH+1]={0};
	MD5(arr.data(), arr.size(), res);

	std::string str_res;
	str_res.reserve(MD5_DIGEST_LENGTH*2+1);
	for(int f=0;f<MD5_DIGEST_LENGTH;++f)
	{
		str_res.push_back(alphabet[res[f]/16]);
		str_res.push_back(alphabet[res[f]%16]);
	}
	return str_res;
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

void DbEngine::check(const leveldb::Status &status)
{
	if (status.ok())
		return;
	err(result_code_t::sError) << status.ToString();
}
