#include "engine.h"
#include "leveldb/db.h"
#include <openssl/md5.h>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include "scope_guard.h"
#include "errors.h"

#include <iostream>
using namespace sofadb;
using namespace leveldb;

revision_info_t::revision_info_t(const std::string &rev)
{
	if (rev.empty())
	{
		id_ = 0;
		rev_ = "";
	} else
	{
		size_t pos = rev.find('-');
		if (pos==std::string::npos || pos==0 || pos==rev.size()-1)
			err(result_code_t::sWrongRevision) << "Invalid revision: " << rev;
		std::string rev_num = rev.substr(0, pos);
		std::string rev_id = rev.substr(pos);

		if (rev_num.find_first_not_of("0123456789")!=std::string::npos)
			err(result_code_t::sWrongRevision) << "Invalid revision num: " << rev;

		id_ = boost::lexical_cast<uint32_t>(rev_num);
		rev_ = std::move(rev_id);
	}
}

std::string revision_info_t::to_string() const
{
	if (id_ == 0)
		return "";

	std::stringstream s;
	s << id_ << '-' << rev_;
	return s.str();
}

void revision_t::calculate_revision()
{
	//[Deleted, OldStart, OldRev, Body, Atts2]
	std::string res=(deleted_?"-":"") + previous_rev_.to_string() +
			json_to_string(json_body_);
	rev_ = revision_info_t(previous_rev_.id_+1,
						   calculate_hash(res.data(), res.size()));
}

Database::Database(DbEngine *parent, const std::string &name)
	: parent_(parent), closed_(false), name_(name),
	  json_meta_(submap_d)
{
	//Instance start time is in nanoseconds
	json_meta_["instance_start_time"].as_int() = time(NULL)*100000;
	json_meta_["db_name"] = name;
}

Database::Database(DbEngine *parent, json_value &&meta)
	: parent_(parent), closed_(false)
{
	json_meta_ = std::move(meta);
	name_ = json_meta_["db_name"].get_str();
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
		database_ptr res(new Database(this, string_to_json(out)));
		databases_[name]=res;
		return res;
	} else
	{
		WriteOptions w;
		database_ptr res(new Database(this, name));
		databases_[name]=res;
		keystore_->Put(w, db_info, json_to_string(res->get_meta()));
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

revision_ptr Database::put(const std::string &id, const maybe_string_t& old_rev,
						   const json_value &json, bool batched)
{
	guard_t g(mutex_);
	check_closed();

	ReadOptions opts;
	WriteOptions wo;
	wo.sync = !batched;

	//Ok. That gets interesting!
	//Let's roll!
	revision_ptr rev(new revision_t());

	rev->id_ = id;
	rev->deleted_ = false;
	if (old_rev)
		rev->previous_rev_=revision_info_t(old_rev.get());
	else
		rev->previous_rev_ = revision_info_t::empty_revision;

	std::pair<json_value, json_value> pair =
			sanitize_and_get_reserved_words(json);
	rev->json_body_ = std::move(pair.first);
	//TODO: attachments
	rev->calculate_revision();
	assert(rev->rev_);

	std::string doc_rev_path=make_path(id, maybe_string_t());

	std::string prev_rev;
	if (parent_->keystore_->Get(opts, doc_rev_path, &prev_rev).ok())
	{
		//There's an existing document.
		if (!old_rev || prev_rev != old_rev.get())
			return revision_ptr(); //Conflict
	}

	//Totaly new document! Calculate its revision
	std::string cur_rev(rev->rev_.get().to_string());
	VLOG_MACRO(1) << "Creating document " << id << " in the database "
			  << name_ << " revid=" << rev->rev_.get() << std::endl;

	parent_->check(parent_->keystore_->Put(wo, doc_rev_path, cur_rev));

	//Store the document itself
	store(wo, rev);

	return rev;
}

std::string Database::make_path(const std::string &id, maybe_string_t rev)
{
	return DbEngine::DATA_DB+"/"+name_+
			DbEngine::DB_SEPARATOR+id+DbEngine::REV_SEPARATOR+
			(rev ? rev.get() : "tip");
}

void Database::store(const WriteOptions &wo, revision_ptr ptr)
{
	std::string doc_data_path=make_path(ptr->id_,
										ptr->rev_.get().to_string());

	json_value serialized(submap_d);
	serialized["deleted"].as_bool() =ptr->deleted_;
	serialized["prev_revid"].as_str() = ptr->previous_rev_.to_string();
	serialized["atts"] = json_value(); //TODO: attachments
	serialized["data"] = ptr->json_body_;

	std::string buf=json_to_string(serialized, false);
	Slice sl(buf);

	parent_->check(
		parent_->keystore_->Put(wo, doc_data_path, sl));
	VLOG_MACRO(1) << "Created document " << ptr->id_ << " in the database "
				<< name_ << ", size=" << buf.size();
}

revision_ptr Database::get(const std::string &id, const maybe_string_t& rev)
{
	//Make sure that we have a stable view. No locking here!
	const Snapshot *snap=parent_->keystore_->GetSnapshot();
	ON_BLOCK_EXIT_OBJ(*parent_->keystore_, &DB::ReleaseSnapshot, snap);

	ReadOptions ro;
	ro.snapshot=snap;

	std::string version;
	if (rev)
		version = rev.get();
	else
	{
		if (!parent_->keystore_->Get(ro, make_path(id, maybe_string_t()),
									 &version).ok())
			return revision_ptr();
	}

	std::string val;
	if (!parent_->keystore_->Get(ro, make_path(id, version), &val).ok())
		return revision_ptr();

	json_value deserialized=string_to_json(val);

	revision_ptr res(new revision_t());
	res->id_ = id;
	res->rev_ = rev;

	res->deleted_ = deserialized["deleted"].get_bool();
	res->previous_rev_ = revision_info_t(deserialized["prev_revid"].get_str());
	//res->atts_; //TODO: attachments
	res->json_body_ = std::move(deserialized["data"]);

	return res;
}
