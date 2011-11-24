#include "engine.h"
#include "database.h"
#include "leveldb/db.h"
#include <openssl/md5.h>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include "scope_guard.h"
#include "errors.h"

#include <iostream>
using namespace sofadb;
using namespace leveldb;

const revision_num_t revision_num_t::empty_revision;

revision_num_t::revision_num_t(uint32_t num, const jstring_t &uniq) :
	num_(num), uniq_(uniq)
{
	char *buf = (char*)alloca(42 + uniq.size());
	sprintf(buf, "%d-%s", num, uniq.c_str());
	stringed_ = buf;
}

revision_num_t::revision_num_t(const jstring_t &rev)
{
	if (rev.empty())
	{
		num_ = 0;
		uniq_ = "";
		stringed_ = "";
	} else
	{
		size_t pos = rev.find('-');
		if (pos==jstring_t::npos || pos==0 || pos==rev.size()-1)
			err(result_code_t::sWrongRevision)
					<< "Invalid revision: " << rev;

		jstring_t rev_num = rev.substr(0, pos);
		jstring_t rev_id = rev.substr(pos+1);

		if (rev_num.find_first_not_of("0123456789")!=jstring_t::npos)
			err(result_code_t::sWrongRevision)
					<< "Invalid revision num: " << rev;

		num_ = boost::lexical_cast<uint32_t>(rev_num);
		uniq_ = std::move(rev_id);
		stringed_ = rev;
	}
}

revision_num_t Database::compute_revision(const revision_num_t &prev,
										const jstring_t &body)
{
	static const char alphabet[17]="0123456789abcdef";
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, body.data(), body.size());
	//TODO: attachments

	unsigned char res[MD5_DIGEST_LENGTH+1]={0};
	MD5_Final(res, &ctx);

	jstring_t str_res;
	str_res.resize(MD5_DIGEST_LENGTH*2);
	for(int f=0;f<MD5_DIGEST_LENGTH;++f)
	{
		str_res[f*2]=alphabet[res[f]/16];
		str_res[f*2+1]=alphabet[res[f]%16];
	}
	return revision_num_t(prev.num()+1, std::move(str_res));
}

Database::Database(DbEngine *parent, const jstring_t &name)
	: parent_(parent), closed_(false), name_(name),
	  json_meta_(submap_d)
{
	//Instance start time is in nanoseconds
	json_meta_["instance_start_time"].as_int() = int64_t(time(NULL))*100000;
	json_meta_["db_name"] = name;
}

Database::Database(DbEngine *parent, json_value &&meta)
	: parent_(parent), closed_(false)
{
	json_meta_ = std::move(meta);
	name_ = json_meta_["db_name"].get_str();
}

void Database::check_closed()
{
	guard_t g(mutex_);
	if (closed_)
		err(result_code_t::sError) << "Database " << name_ << " is closed";
}

std::pair<json_value, json_value>
	Database::sanitize_and_get_reserved_words(const json_value &tp)
{
	json_value sanitized(submap_d);
	json_value special(submap_d);

	for(auto i=tp.get_submap().begin(), ie=tp.get_submap().end(); i!=ie; ++i)
	{
		if (i->first.at(0) == '_')
			special[i->first] = i->second;
		else
			sanitized[i->first] = i->second;
	}

	return std::make_pair(std::move(sanitized), std::move(special));
}

revision_t Database::put(const jstring_t &id, const revision_num_t& old_rev,
						const json_value &meta, const json_value &content,
						bool batched)
{
	guard_t g(mutex_);
	check_closed();

	ReadOptions opts;
	WriteOptions wo;
	wo.sync = !batched;

	//Ok. That gets interesting!
	//Let's roll!
	revision_t rev;

	//Check if there is an old revision with this ID
	std::string prev_rev;
	std::string doc_rev_path=make_path(id, 0);
	if (parent_->keystore_->Get(opts, doc_rev_path, &prev_rev).ok())
	{
		//There's an existing document.
		if (old_rev.empty() || prev_rev != old_rev.full_string())
			return rev;
			//return revision_ptr(); //Conflict
	}
	//Update or create a document!

	rev.id_ = id;
	rev.deleted_ = false;
	if (!old_rev.empty())
		rev.previous_rev_ = old_rev;
	else
		rev.previous_rev_ = revision_num_t::empty_revision;

	json_value serialized(submap_d);
	serialized["deleted"].as_bool() =rev.deleted_;
	serialized["prev_revid"].as_str() = rev.previous_rev_.full_string();
	serialized["atts"] = json_value(); //TODO: attachments
	serialized["data"].as_graft() = &content;

	jstring_t body=json_to_string(serialized);
	rev.rev_ = compute_revision(rev.previous_rev_, body);
	//TODO: attachments
	assert(!rev.rev_.empty());

	//Write tip pointer
	parent_->check(
		parent_->keystore_->Put(wo, doc_rev_path, rev.rev_.full_string()));

	std::string doc_data_path=make_path(rev.id_, &rev.rev_.full_string());
	//Write the document
	parent_->check(parent_->keystore_->Put(wo, doc_data_path, body));

	VLOG_MACRO(1) << "Created document " << id << " in the database "
			  << name_ << " revid=" << rev.rev_ << std::endl;

	return std::move(rev);
}

jstring_t Database::make_path(const jstring_t &id, const std::string *rev)
{
	char *buf = (char*)alloca(name_.size() + id.size()
							  + (rev?rev->size():0) + 42);
	sprintf(buf, SD_DATA_DB "/%s" DB_SEPARATOR "%s" REV_SEPARATOR "%s",
			name_.c_str(), id.c_str(), rev ? rev->c_str() : "tip");
	return buf;
}


Database::res_t Database::get(const jstring_t &id, const revision_num_t& rev)
{
	//Make sure that we have a stable view. No locking here!
	const Snapshot *snap=parent_->keystore_->GetSnapshot();
	ON_BLOCK_EXIT_OBJ(*parent_->keystore_, &DB::ReleaseSnapshot, snap);

	ReadOptions ro;
	ro.snapshot=snap;

	std::pair<revision_t, json_value> res(
				std::make_pair(revision_t(), json_value()));

	jstring_t path;
	if (rev.empty())
	{
		jstring_t version;
		if (!parent_->keystore_->Get(ro, make_path(id, 0), &version).ok())
			return std::move(res);
		path = make_path(id, &version);

		res.first.id_=id;
		res.first.rev_ = revision_num_t(version);
	} else
	{
		res.first.id_ = id;
		res.first.rev_ = rev;
		path = make_path(id, &rev.full_string());
	}

	jstring_t val;
	parent_->check(parent_->keystore_->Get(ro, path, &val));

	json_value serialized=string_to_json(val);
	res.first.deleted_ = serialized["deleted"].get_bool();
	res.first.previous_rev_ = revision_num_t(serialized["prev_revid"].get_str());
	//serialized["atts"] = json_value(); //TODO: attachments

	res.second = std::move(serialized["data"]);

	return std::move(res);
}
