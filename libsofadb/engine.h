#ifndef ENGINE_H
#define ENGINE_H
#include "common.h"

#include "json_spirit.h"

#include <map>
#include <list>

namespace leveldb {
	class DB;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {
	class DbEngine;

	class InlineAttachment
	{
		std::string name_, content_type_;
		long long len_;
	};
	typedef std::map<std::string, InlineAttachment> attachment_map_t;

	class RevisionInfo
	{
	};

	class DocumentRevision
	{
		DbEngine *engine_;

		std::string id_;
		std::string rev_;
		bool deleted_;

		std::list<std::string> conflicts_;
		std::list<std::string> deleted_conflicts_;
		std::list<RevisionInfo> _revs_info;
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
