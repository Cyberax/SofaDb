#ifndef SOFA_DATABASE_H
#define SOFA_DATABASE_H

#include "common.h"
#include "erlang_compat.h"
#include <boost/optional.hpp>

namespace leveldb {
	class DB;
	class Status;
	class WriteOptions;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {

	class DbEngine;
	typedef boost::optional<std::string> maybe_string_t;

	class inline_attachment_t
	{
		std::string name_, content_type_;
		uint8_t md5_[16];
	};
	typedef std::vector<inline_attachment_t> attachment_vector_t;

	struct revision_info_t
	{
		SOFADB_PUBLIC static const revision_info_t empty_revision;

		uint32_t id_; //This is a revision number
		std::string rev_; //Document's MD5 hash or other ID

		revision_info_t() : id_() {}
		SOFADB_PUBLIC revision_info_t(const std::string &);
		SOFADB_PUBLIC std::string to_string() const;
	};

	inline std::ostream& operator << (std::ostream& str,
									  const revision_info_t &r)
	{
		str << r.to_string();
		return str;
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
		std::string id_; //The immutable document ID

		bool deleted_;
		//Revision in the format num-MD5 where MD5 is a hash of the document's
		//contents.
		revision_info_t previous_rev_;
		erlang::erl_type_t json_body_;
		attachment_vector_t atts_;

		//Cached revision info, can be computed based on the previous
		//revision
		boost::optional<revision_info_t> rev_;
		SOFADB_PUBLIC void calculate_revision();

		maybe_string_t get_rev() const
		{
			if (!rev_)
				return maybe_string_t();
			return maybe_string_t(rev_.get().to_string());
		}
	};
	typedef boost::shared_ptr<revision_t> revision_ptr;

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
		erlang
		::erl_type_t json_meta_;
		std::string name_;

		Database(DbEngine *parent, const std::string &name);
		Database(DbEngine *parent, const erlang::erl_type_t &meta);

		friend class DbEngine;
	public:
		const erlang::erl_type_t& get_meta() const {return json_meta_;}

		bool operator == (const Database &other) const
		{
			return deep_eq(other.json_meta_, json_meta_);
		}

		SOFADB_PUBLIC revision_ptr get(
			const std::string &id, const maybe_string_t& rev=maybe_string_t());
		SOFADB_PUBLIC revision_ptr put(
			const std::string &id, const maybe_string_t& rev,
			const erlang::erl_type_t &json, bool batched);
		SOFADB_PUBLIC revision_ptr remove(
			const std::string &id, const maybe_string_t& rev,
			bool batched);
		SOFADB_PUBLIC revision_ptr copy(
			const std::string &id,
			const maybe_string_t& rev,
			const std::string &dest_id,
			const maybe_string_t& dest_rev,
			bool batched);

	private:
		void check_closed();
		std::pair<erlang::erl_type_t, erlang::erl_type_t>
			sanitize_and_get_reserved_words(const erlang::erl_type_t &tp);

		void store(const leveldb::WriteOptions &wo, revision_ptr ptr);
		std::string make_path(const std::string &id, maybe_string_t rev);
	};
	typedef boost::shared_ptr<Database> database_ptr;

}; //namespace sofadb

#endif // SOFA_DATABASE_H
