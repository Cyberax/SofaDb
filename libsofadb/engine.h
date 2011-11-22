#ifndef ENGINE_H
#define ENGINE_H
#include "database.h"

#include <map>

namespace sofadb {

	SOFADB_PUBLIC std::string calculate_hash(
		const std::vector<unsigned char> &arr);

	class DbEngine
	{
		friend class Database;

		const static std::string SYSTEM_DB;
		const static std::string DATA_DB;
		const static char DB_SEPARATOR='!';
		const static char REV_SEPARATOR='@';

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

	private:
		void check(const leveldb::Status &status);
	};
};

#endif // ENGINE_H
