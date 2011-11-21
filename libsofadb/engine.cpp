#include "engine.h"
#include "leveldb/db.h"
#include "erlang_json.h"
#include <openssl/md5.h>
#include <time.h>
#include <boost/lexical_cast.hpp>
#include "erlang_compat.h"
#include "errors.h"
#include <glog/logging.h>

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

revision_info_t::revision_info_t(const std::string &rev)
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
	list_ptr_t head=(list_t::make());
	list_ptr_t lst=head;
	lst->val_ = deleted_ ? atom_t::TRUE : atom_t::FALSE;
	lst->tail_ = list_t::make(); lst=get_list(lst->tail_);

	lst->val_ = BigInteger(previous_rev_.id_);
	lst->tail_ = list_t::make(); lst=get_list(lst->tail_);

	if (previous_rev_.id_!=0)
		lst->val_ = binary_t::make_from_string(previous_rev_.rev_);
	else
		lst->val_ = BigInteger(0);
	lst->tail_ = list_t::make(); lst=get_list(lst->tail_);

	lst->val_ = json_body_;
	lst->tail_ = list_t::make(); lst=get_list(lst->tail_);

	lst->val_ = erl_nil; //TODO: attachments
	lst->tail_ = erl_nil;

	utils::buf_stream str;
	term_to_binary(head, &str);

	revision_info_t new_rev;
	new_rev.id_ = previous_rev_.id_ + 1;
	new_rev.rev_ = calculate_hash(str.buffer);

	binary_ptr_t bin=binary_t::make();
	bin->binary_ = str.buffer;

	rev_ = std::move(new_rev);
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

Database::Database(DbEngine *parent, const std::string &name)
	: parent_(parent), closed_(false), name_(name)
{
	json_meta_=create_submap();
	//Instance start time is in nanoseconds
	put_val(json_meta_, "instance_start_time") = BigInteger(time(NULL))*100000;
	put_val(json_meta_, "db_name") = binary_t::make_from_string(name);
}

Database::Database(DbEngine *parent,
				   const erl_type_t &meta)
	: parent_(parent), closed_(false)
{
	json_meta_ = parse_json(json_to_string(meta));
	name_ = binary_to_string(get_val(json_meta_, "db_name"));
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
		database_ptr res(new Database(this, parse_json(out)));
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

std::pair<erl_type_t,erl_type_t>
	Database::sanitize_and_get_reserved_words(const erl_type_t &tp)
{
	erl_type_t sanitized = create_submap();
	erl_type_t special = create_submap();

	//list_ptr_t prev;
	list_ptr_t cur=get_json_list(tp);
	assert(cur);
	do
	{
		tuple_ptr_t ptr=boost::get<tuple_ptr_t>(cur->val_);
		assert(ptr->elements_.size()==2);

		const binary_ptr_t &key=boost::get<binary_ptr_t>(ptr->elements_.at(0));

		erl_type_t val=deep_copy(ptr->elements_.at(1)); //Make a deep copy
		if (key->binary_.at(0)=='_')
			put_val(special, binary_to_string(key)) = std::move(val);
		else
			put_val(sanitized, binary_to_string(key)) = std::move(val);
	} while(advance(&cur));
	assert(is_nil(cur->tail_));

	return std::make_pair(std::move(sanitized), std::move(special));
}

revision_ptr Database::put(const std::string &id, const maybe_string_t& old_rev,
						   const erlang::erl_type_t &json, bool batched)
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

	std::pair<erl_type_t, erl_type_t> pair =
			sanitize_and_get_reserved_words(json);
	rev->json_body_ = std::move(pair.first);
	//TODO: attachments
	rev->calculate_revision();
	assert(rev->rev_);

	std::string doc_rev_path=DbEngine::DATA_DB+"/"+name_+
			DbEngine::DB_SEPARATOR+id+DbEngine::REV_SEPARATOR;

	std::string prev_rev;
	if (parent_->keystore_->Get(opts, doc_rev_path, &prev_rev).ok())
	{
		//There's an existing document.
	} else
	{
		//Totaly new document! Calculate its revision
		std::string cur_rev(rev->rev_.get().to_string());
		LOG(INFO) << "Creating document " << id << " in the database "
				  << name_ << " revid=" << rev->rev_.get() << std::endl;

		parent_->check(
					parent_->keystore_->Put(wo, doc_rev_path+"tip", cur_rev));

		//Store the document itself
		store(wo, rev);
	}

	return rev;
}

void Database::store(const WriteOptions &wo, revision_ptr ptr)
{
	std::string doc_data_path=DbEngine::DATA_DB+"/"+name_+
			DbEngine::DB_SEPARATOR+ptr->id_+DbEngine::REV_SEPARATOR+
			ptr->rev_.get().to_string();

	erl_type_t serialized = create_submap();
	put_val(serialized, "deleted") = atom_t::convert(ptr->deleted_);
	put_val(serialized, "prev_revid") = binary_t::make_from_string(
			ptr->previous_rev_.to_string());
	put_val(serialized, "atts") = erl_nil; //TODO: attachments
	put_val(serialized, "data") = ptr->json_body_;

	std::string buf=json_to_string(serialized, false); //TODO: binary format?
	parent_->check(
		parent_->keystore_->Put(wo, doc_data_path, buf));
	LOG(INFO) << "Created document " << ptr->id_ << " in the database "<< name_
			  << ", size=" << buf.size();
}
