#ifndef ENGINE_H
#define ENGINE_H
#include "common.h"

#include "json_spirit.h"

#include <map>
#include <list>
#include <boost/optional.hpp>

namespace leveldb {
	class DB;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {
	class DbEngine;

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
		DbEngine *engine_;

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

	class DbEngine
	{
		leveldb::db_ptr_t keystore_;
	public:
		SOFADB_PUBLIC DbEngine(const std::string &&filename);
		SOFADB_PUBLIC virtual ~DbEngine();
	};
};

#endif // ENGINE_H
