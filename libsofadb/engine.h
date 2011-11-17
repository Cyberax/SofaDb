#ifndef ENGINE_H
#define ENGINE_H
#include "common.h"

#include "json_spirit.h"

#include <map>
#include <set>
#include <boost/optional.hpp>

namespace leveldb {
	class DB;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {
	class DbEngine;

	typedef boost::optional<std::string> maybe_string_t;

	class InlineAttachment
	{
		std::string name_, content_type_;
		uint8_t md5_[16];
	};
	typedef std::vector<InlineAttachment> attachment_vector_t;

	struct RevisionInfo
	{
		uint32_t id_; //This is a revision number
		uint8_t md5_[16]; //Document's MD5 hash

		RevisionInfo() : md5_(), id_()
		{
		}
	};

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

		Hash is computed using a braindead algorithm and represents MD5 of the
		original attachment OR of its zlib-compressed copy. No compression
		flags are stored, but by default the following documents are compressed
		by zlib with 8 compression level:
		text/*, application/javascript, application/json, application/xml
	  */
	class DocumentRevision
	{
		std::string id_; //The immutable document ID

		bool deleted_;
		//Revision in the format num-MD5 where MD5 is a hash of the document's
		//contents.
		RevisionInfo previous_rev_;
		json_spirit::Array body_;
		attachment_vector_t atts_;

		//Cached revision info, can be computed based on the previous
		//revision
		boost::optional<RevisionInfo> rev_;
	};
	typedef boost::shared_ptr<DocumentRevision> revision_ptr;

	/**
		Database has the following metadata present. Not everything is yet
		implemented.
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
		time_t created_on_;
		std::string name_;

		Database(DbEngine *parent,
				 time_t created_on, const std::string &name);
		Database(DbEngine *parent,
				 const json_spirit::Object &json);

		friend class DbEngine;
	public:
		SOFADB_PUBLIC json_spirit::Object to_json() const;

		bool operator == (const Database &other) const
		{
			return created_on_==other.created_on_ && other.name_==name_;
		}

		SOFADB_PUBLIC revision_ptr get(
			const std::string &id, const maybe_string_t& rev);
		SOFADB_PUBLIC revision_ptr put(
			const std::string &id, const maybe_string_t& rev,
			const std::string &doc_str, bool batched);
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
	};
	typedef boost::shared_ptr<Database> database_ptr;

	class DbEngine
	{
		friend class Database;

		const static std::string SYSTEM_DB;
		const static std::string DATA_DB;
		const static char DB_SEPARATOR='!';
		const static char REV_SEPARATOR=':';

		leveldb::db_ptr_t keystore_;
		bool temporary_;
		std::string filename_;

		std::map<std::string, database_ptr> databases_;
		std::recursive_mutex mutex_;

	public:
		SOFADB_PUBLIC DbEngine(const std::string &filename, bool temporary);
		SOFADB_PUBLIC virtual ~DbEngine();

		SOFADB_PUBLIC void checkpoint();
		SOFADB_PUBLIC database_ptr create_a_database(const std::string &name);
	};
};

#endif // ENGINE_H
