#ifndef ENGINE_H
#define ENGINE_H
#include "common.h"

#include "json_spirit.h"

#include <map>
#include <set>
#include <boost/optional.hpp>
#include <mutex>

namespace leveldb {
	class DB;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {
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

	struct DatabaseInfo
	{
		time_t created_on_;
		std::string name_;

		DatabaseInfo() : created_on_(), name_() {}
		DatabaseInfo(const json_spirit::Object &json);
		json_spirit::Object to_json() const;

		bool operator == (const DatabaseInfo &other) const
		{
			return created_on_==other.created_on_ && other.name_==name_;
		}
	};

	class DbEngine
	{
		const static std::string SYSTEM_DB;
		const static char DB_SEPARATOR='!';

		leveldb::db_ptr_t keystore_;
		std::map<std::string, DatabaseInfo> databases_;
		std::recursive_mutex mutex_;
		typedef std::lock_guard<std::recursive_mutex> guard_t;

	public:
		SOFADB_PUBLIC DbEngine(const std::string &&filename);
		SOFADB_PUBLIC virtual ~DbEngine();

		SOFADB_PUBLIC void checkpoint();
		SOFADB_PUBLIC DatabaseInfo create_a_database(const std::string &name);
	};
};

#endif // ENGINE_H
