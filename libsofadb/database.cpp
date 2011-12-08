#include "database.h"
#include "storage_interface.h"
#include "json_stream.h"
#include <openssl/md5.h>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include "scope_guard.h"
#include "errors.h"
#include "conflict.h"

#include <iostream>
using namespace sofadb;
using namespace leveldb;

const revision_num_t revision_num_t::empty_revision;

revision_num_t::revision_num_t(uint32_t num, jstring_t &&uniq) :
	num_(num), uniq_(std::move(uniq))
{
	stringed_.reserve(uniq_.size()+16);
	append_int_to_string(num, stringed_);
	stringed_.push_back('-');
	stringed_.append(uniq_);
}

revision_num_t::revision_num_t(uint32_t num, const jstring_t &uniq) :
	num_(num), uniq_(uniq)
{
	stringed_.reserve(uniq_.size()+16);
	append_int_to_string(num, stringed_);
	stringed_.push_back('-');
	stringed_.append(uniq);
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

		num_ = atoi(rev_num.c_str());//boost::lexical_cast<uint32_t>(rev_num);
		uniq_ = std::move(rev_id);
		stringed_ = rev;
	}
}

revision_num_t Database::compute_revision(const revision_num_t &prev,
										const jstring_t &body)
{
	static const char alphabet[17]="0123456789abcdef";
	//TODO: attachments

	unsigned char res[MD5_DIGEST_LENGTH+1]={0};
	MD5((const unsigned char*)body.data(), body.size(), res);

	jstring_t str_res;
	str_res.resize(MD5_DIGEST_LENGTH*2);
	for(int f=0;f<MD5_DIGEST_LENGTH;++f)
	{
		str_res[f*2]=alphabet[res[f]/16];
		str_res[f*2+1]=alphabet[res[f]%16];
	}
	return revision_num_t(prev.num()+1, std::move(str_res));
}

Database::Database(const jstring_t &name)
	: closed_(false), name_(name), json_meta_(submap_d)
{
	//Instance start time is in nanoseconds
	json_meta_["instance_start_time"].as_int() = int64_t(time(NULL))*100000;
	json_meta_["db_name"] = name;
}

Database::Database(json_value &&meta)
	: closed_(false)
{
	json_meta_ = std::move(meta);
	name_ = json_meta_["db_name"].get_str();
}

void Database::check_closed()
{
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

bool Database::get_revlog(storage_t *ifc,
					   const jstring_t &path_base, json_value &res)
{
	jstring_t rev_info_log;
	if (ifc->try_get(path_base, &rev_info_log))
	{
		//There's an existing document.
		res = string_to_json(rev_info_log);
		return true;
	} else
	{
		revlog_wrapper(res).init();
		return false;
	}
}

put_result_t Database::put(storage_t *ifc,
				   const jstring_t &id, const revision_num_t& old_rev,
				   const json_value &content, bool do_merge)
{
	check_closed();
	//Ok. That gets interesting!
	//Let's roll!
	put_result_t put_res;

	//Check if there is an old revision with this ID
	const std::string doc_rev_path_base=make_path(id);

	bool has_prev = get_revlog(ifc, doc_rev_path_base, put_res.rev_log_);
	bool need_to_merge = false;
	if (has_prev)
	{
		sublist_t& quad = revlog_wrapper(put_res.rev_log_).top_quad();
		if (old_rev.num()!=quad.at(0).get_int() ||
				old_rev.uniq()!=quad.at(1).get_str())
		{
			//Conflict!
			if (!do_merge)
			{
				put_res.code_ = UPDATE_CONFLICT;
				return std::move(put_res);
			}
			need_to_merge = true;
		}
	}
	//Update or create a document!
	put_res.assigned_rev_ = store_data(ifc, doc_rev_path_base, old_rev,
						  false, content);
	assert(!put_res.assigned_rev_.empty());

	//Format the revlog
	if (!need_to_merge)
	{
		revlog_wrapper w(put_res.rev_log_);
		if (!old_rev.empty() && !has_prev)
		{
			//We have a prior but missing revision
			w.add_rev_info(old_rev, false, false);
		}
		w.add_rev_info(put_res.assigned_rev_, true, false);
	} else
	{
		sublist_t& quad = revlog_wrapper(put_res.rev_log_).top_quad();
		//Merge it!
		resolver reslv(&put_res.rev_log_);
		reslv.merge(put_res.assigned_rev_, sublist_t());
	}

	//Write the revlog info
	ifc->put(doc_rev_path_base, json_to_string(put_res.rev_log_));

	VLOG_MACRO(1) << "Created document " << id << " in the database "
				  << name_ << " revid=" << put_res.assigned_rev_ << " at "
				  << doc_rev_path_base << "..." << std::endl;

	return std::move(put_res);
}

class optionaly_pooled_alloc
{

};

revision_num_t Database::store_data(storage_t *ifc,
									const jstring_t &doc_data_path_base,
									const revision_num_t &prev_rev_,
									bool deleted,
									const json_value &content)
{
	jstring_t body;
	body.reserve(128);

	//Format is [deleted, prev_rev, attachments, content]
	std::auto_ptr<json_stream> str=make_stream(body, false);
	str->start_list();
	str->write_bool(deleted);
	str->write_string(prev_rev_.full_string());
	//TODO: attachments
	str->write_null();
	str->write_json(content);
	str->end_list();

	revision_num_t rev=compute_revision(prev_rev_, body);

	//Write the document
	std::string doc_data_path=doc_data_path_base+rev.full_string();
	ifc->put(doc_data_path, body);

	//TODO: attachments
	return rev;
}

jstring_t Database::make_path(const jstring_t &id)
{
	//Optimized, so it's ugly.
	jstring_t res;
	res.reserve(name_.size() + id.size() + 42);

	res.append(SD_DATA_DB"/", sizeof(SD_DATA_DB"/"));
	res.append(name_);
	res.append(DB_SEPARATOR, sizeof(DB_SEPARATOR));
	res.append(id);
	res.append(REV_SEPARATOR);
	return res;
}

bool Database::get(storage_t *ifc,
				   const jstring_t &id, const revision_num_t *rev_num,
				   json_value *content, revision_t *rev, json_value *rev_log)
{
	assert(content || rev || rev_log); //At least something must be present!
	jstring_t path_base = make_path(id);

	revision_num_t num;
	if (!rev_num || rev_log)
	{
		//We need to query the revision history either if we are interested
		//in it or if we don't know it.
		jstring_t version_log;
		if (!ifc->try_get(path_base, &version_log))
			return false;

		json_value log = string_to_json(version_log);
		if (rev_log)
			*rev_log = std::move(log);
		if (!content && !rev) //We're not interested in further info
			return true;

		//Otherwise get the last revision
		if (!rev_num)
		{
			num = revlog_wrapper(log).top_rev_id();
		} else
			num = *rev_num;
	} else
	{
		assert(rev_num);
		num = *rev_num;
	}

	jstring_t val;
	if (!ifc->try_get(path_base+num.full_string(), &val))
	{
		//Note, previous version used MVCC snapshots. However, it's
		//not strictly necessary here - stored documents are immutable and
		//can't change between the version query and this point.
		//However, it's theoretically possible that pruning can occur
		//between getting the version and this point.
		if (rev_num)
			VLOG_MACRO(1) << "Pruned database during document retreival, ID="
						  << id
						  << std::endl;
		return false; //Revision was not found :(
	}

	json_value serialized=string_to_json(val);
	sublist_t &lst = serialized.get_sublist();
	//Format is [deleted, prev_rev, attachments, content]
	if (content)
		*content = std::move(lst.at(3));

	if (rev)
	{
		rev->id_ = id;
		rev->deleted_ = lst.at(0).get_bool();
		rev->previous_rev_ = revision_num_t(lst.at(1).get_str());
		//serialized["atts"] = json_value(); //TODO: attachments
		rev->rev_ = num;
	}

	return true;
}
