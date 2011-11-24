#ifndef SOFA_DATABASE_H
#define SOFA_DATABASE_H

#include "common.h"
#include "native_json.h"
#include <boost/optional.hpp>

#define SD_SYSTEM_DB "_sys"
#define SD_DATA_DB "_data"
#define DB_SEPARATOR "!"
#define REV_SEPARATOR "@"

namespace leveldb {
	class DB;
	class Status;
	class WriteOptions;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {

	class DbEngine;
	typedef boost::optional<jstring_t> maybe_string_t;

	class inline_attachment_t
	{
		jstring_t name_, content_type_;
		uint8_t md5_[16];
	};
	typedef std::vector<inline_attachment_t> attachment_vector_t;

	class revision_num_t
	{
		jstring_t stringed_;
		uint32_t num_; //This is a revision number
		jstring_t uniq_; //Document's MD5 hash or other unique ID

	public:
		SOFADB_PUBLIC static const revision_num_t empty_revision;

		revision_num_t() : num_() {}
		revision_num_t(uint32_t num, const jstring_t &uniq);
		revision_num_t(const jstring_t &);

		revision_num_t(const revision_num_t &o) :
			stringed_(o.stringed_), num_(o.num_), uniq_(o.uniq_)
		{

		}
		revision_num_t(revision_num_t &&o) :
			stringed_(std::move(o.stringed_)), num_(o.num_), uniq_(o.uniq_)
		{
		}
		revision_num_t& operator = (revision_num_t &&o)
		{
			if (this == &o) return *this;
			stringed_=std::move(o.stringed_);
			num_=o.num_;
			uniq_=o.uniq_;
			return *this;
		}
		revision_num_t& operator = (const revision_num_t &o)
		{
			stringed_=o.stringed_;
			num_=o.num_;
			uniq_=o.uniq_;
			return *this;
		}

		bool empty() const { return num_==0; }

		const jstring_t& full_string() const { return stringed_; };
		uint32_t num() const { return num_; }
		const jstring_t& uniq() const { return uniq_; }
	};

	inline bool operator == (const revision_num_t &l, const revision_num_t &r)
	{
		return l.num() == r.num() && l.uniq() == r.uniq();
	}

	inline std::ostream& operator << (std::ostream& str,
									  const revision_num_t &r)
	{
		return str << r.full_string();
	}

	/**
		CouchDB keeps documents in the following format:
		[Deleted, OldStart, OldRev, Body, Atts2] where
		- 'Deleted' is either 'true' or 'false' atoms
		- OldStart - the previous revision number as an integer
		- OldRev - the MD5 of the previous revision, as a binary
		- Body - the body of the document
		- Atts2 - inline attachments

		Body has the following structure:
		- {[{"prop_name", prop_value}, ...]} where prop_name is a binary
		and prop_value is an Erlang term. This is either a:
		+ 'null' atom for nulls
		+ Number/double for integers/doubles
		+ Binary for string values
		+ A tuple containing further JSON tuples in the same format.

		Atts2 describes attachments and has the following structure:
		[{<<file_name>>,<<content_type>>, <<md5>>}, ...] where
		file_name and content_type are binaries and md5	is a binary with the
		hash of the content.

		Hash is computed using a curious algorithm and represents MD5 of the
		original attachment OR of its zlib-compressed copy. No compression
		flags are stored, but by default the following documents are compressed
		by zlib with 8 compression level:
		text/*, application/javascript, application/json, application/xml
	  */
	struct revision_t
	{
		jstring_t id_; //The immutable document ID
		bool deleted_;

		revision_num_t previous_rev_;
		attachment_vector_t atts_;

		//Cached revision info, can be computed based on the previous
		//revision
		revision_num_t rev_;

		bool empty() const { return id_.empty(); }
	};

	/**
		Database should have the following metadata present.
		Not everything is yet implemented.
		 {
		"db_name": "test",
		"doc_count":2,
		"doc_del_count":2,
		"update_seq":34,
		"purge_seq":0,
		"compact_running":false,
		"disk_size":131163,
		"instance_start_time":"1321264006959501",
		"disk_format_version":5,
		"committed_update_seq":34
		}
	*/
	class Database
	{
		DbEngine *parent_;
		std::recursive_mutex mutex_;

		bool closed_;
		json_value json_meta_;
		jstring_t name_;

		Database(DbEngine *parent, const jstring_t &name);
		Database(DbEngine *parent, json_value &&meta);

		friend class DbEngine;
	public:
		typedef std::pair<revision_t, json_value> res_t;

		const json_value& get_meta() const {return json_meta_;}

		bool operator == (const Database &other) const
		{
			return other.json_meta_ == json_meta_;
		}

		SOFADB_PUBLIC res_t get(const jstring_t &id,
			const revision_num_t& = revision_num_t::empty_revision);
		SOFADB_PUBLIC revision_t put(
			const jstring_t &id, const revision_num_t& old_rev,
			const json_value &meta, const json_value &content, bool batched);
		SOFADB_PUBLIC revision_t remove(
			const jstring_t &id, const revision_num_t& rev, bool batched);
		SOFADB_PUBLIC revision_t copy(
			const jstring_t &id, const revision_num_t &rev,
			const jstring_t &dest_id, const revision_num_t &dest_rev,
			bool batched);

		std::pair<json_value, json_value>
			sanitize_and_get_reserved_words(const json_value &tp);
	private:
		void check_closed();

		jstring_t make_path(const jstring_t &id, const std::string *rev);
		revision_num_t compute_revision(
			const revision_num_t &prev, const jstring_t &body);
	};

}; //namespace sofadb

#endif // SOFA_DATABASE_H
