#ifndef ENGINE_H
#define ENGINE_H
#include "common.h"
#include <leveldb/db.h>
#include "storage_interface.h"

#include <map>

namespace leveldb {
	class DB;
	class Status;
	class WriteOptions;
	typedef boost::shared_ptr<DB> db_ptr_t;
};

namespace sofadb {

	class Database;
	typedef boost::shared_ptr<Database> database_ptr;

	class DbEngine
	{
		friend class Database;

		leveldb::db_ptr_t keystore_;
		bool temporary_;
		jstring_t filename_;

		std::map<jstring_t, database_ptr> databases_;
		std::recursive_mutex mutex_;

	public:
		SOFADB_PUBLIC DbEngine(const jstring_t &filename, bool temporary);
		SOFADB_PUBLIC virtual ~DbEngine();

		SOFADB_PUBLIC void checkpoint();
		SOFADB_PUBLIC database_ptr create_a_database(const jstring_t &name);

		SOFADB_PUBLIC storage_ptr_t create_storage(bool sync);
		//SOFADB_PUBLIC batch_storage_ptr_t create_batch_storage();

		static void check(const leveldb::Status &status);
	};

	typedef boost::shared_ptr<DbEngine> engine_ptr;
};

#endif // ENGINE_H
