#ifndef SOFA_DATABASE_H
#define SOFA_DATABASE_H

#include "common.h"
#include "native_json.h"
#include "boilerplate.hpp"

#define SD_SYSTEM_DB "_sys"
#define SD_DATA_DB "_data"
#define DB_SEPARATOR "!"
#define REV_SEPARATOR "@"

namespace leveldb {
	class DB;
	class Status;
	class WriteOptions;
	class WriteBatch;
	typedef boost::shared_ptr<DB> db_ptr_t;
	class WriteBatch;
	typedef boost::shared_ptr<WriteBatch> batch_ptr_t;
};

namespace sofadb {
	class storage_t;

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
		SOFADB_PUBLIC revision_num_t(uint32_t num, jstring_t &&uniq);
		SOFADB_PUBLIC revision_num_t(uint32_t num, const jstring_t &uniq);
		SOFADB_PUBLIC revision_num_t(const jstring_t &);

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
	inline bool operator != (const revision_num_t &l, const revision_num_t &r)
	{
		return !(l==r);
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

		revision_t() : deleted_() {}
		UTILITY_MOVE_DEFAULT_MEMBERS(
			revision_t, (id_)(deleted_)(previous_rev_)(atts_)(rev_))
	};

	enum update_status_e { UPDATE_OK, UPDATE_CONFLICT,
						UPDATE_CONFLICT_WON, UPDATE_CONFLICT_LOST };

	struct put_result_t
	{
		update_status_e code_;
		revision_num_t assigned_rev_;
		json_value rev_log_;

		UTILITY_MOVE_DEFAULT_MEMBERS(
			put_result_t, (code_)(assigned_rev_)(rev_log_))
		put_result_t() : code_() {}
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
		bool closed_;
		json_value json_meta_;
		jstring_t name_;

		Database(const jstring_t &name);
		Database(json_value &&meta);

		friend class DbEngine;
	public:
		const json_value& get_meta() const {return json_meta_;}

		bool operator == (const Database &other) const
		{
			return other.json_meta_ == json_meta_;
		}

		SOFADB_PUBLIC bool get(storage_t *ifc,
							   const jstring_t &id,
							   const revision_num_t *rev_num,
							   json_value *content,
							   revision_t *rev=0,
							   json_value *rev_log=0);

		SOFADB_PUBLIC put_result_t put(storage_t *ifc,
			const jstring_t &id, const revision_num_t& old_rev,
			const json_value &content, bool do_merge = false);

		/*
		SOFADB_PUBLIC revision_t remove(
			const jstring_t &id, const revision_num_t& rev,
			leveldb::batch_ptr_t batch = leveldb::batch_ptr_t());
		SOFADB_PUBLIC revision_t copy(
			const jstring_t &id, const revision_num_t &rev,
			const jstring_t &dest_id, const revision_num_t &dest_rev,
			leveldb::batch_ptr_t batch=leveldb::batch_ptr_t());
		*/

		std::pair<json_value, json_value>
			sanitize_and_get_reserved_words(const json_value &tp);
	private:
		void check_closed();

		bool get_revlog(storage_t *ifc,
					 const jstring_t &path_base, json_value &res);
		revision_num_t store_data(storage_t *ifc,
								  const jstring_t &doc_data_path_base,
								  const revision_num_t &prev_rev,
								  bool deleted,
								  const json_value &content);

		jstring_t make_path(const jstring_t &id);
		revision_num_t compute_revision(
			const revision_num_t &prev, const jstring_t &body);
	};

	/**
		The structure of revlog is:
		[ ["1-conflicted-id", "2-conflicted-id", ...],
		  [0, "id", true], ["1", "id2", true],..
		]

		Where "confliced" strings are lists of conflicted revisions and
		[num, "id", available] triples describe the stored revisions
	  */
	struct revlog_wrapper
	{
		json_value &log_;
		revlog_wrapper(json_value &val) : log_(val) {}

		void init()
		{
			assert(log_.is_nil());
			sublist_t &sub = log_.as_sublist();
			sub.reserve(3);
			//Reserve the first tuple for conflicts info
			sub.push_back(json_value(sublist_d));
			//The second tuple for deleted conflicts
			sub.push_back(json_value(sublist_d));
		}

		void add_rev_info(const revision_num_t &rev,
						  bool available, bool is_deleted)
		{
			json_value quad(sublist_d);
			sublist_t &sub = quad.get_sublist();
			sub.reserve(4);
			sub.push_back(json_value(int64_t(rev.num())));
			sub.push_back(rev.uniq());
			sub.push_back(json_value(available));
			sub.push_back(json_value(is_deleted));

			log_.as_sublist().push_back(std::move(quad));
		}

		revision_num_t top_rev_id() const
		{
			const sublist_t& quad=log_.get_sublist().back().get_sublist();
			return revision_num_t(quad.at(0).get_int(),
								  quad.at(1).get_str());
		}

		sublist_t& top_quad()
		{
			return log_.get_sublist().back().get_sublist();
		}

		void add_conflict(const jstring_t &rev)
		{
			log_.as_sublist().at(0).get_sublist().push_back(rev);
		}

		sublist_t& get_conflicts()
		{
			return log_.as_sublist().at(0).get_sublist();
		}
	};
}; //namespace sofadb

#endif // SOFA_DATABASE_H
