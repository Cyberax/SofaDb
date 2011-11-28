#ifndef STORAGE_INTERFACE
#define STORAGE_INTERFACE

#include "common.h"
/*
 ReadOptions opts;
 WriteOptions wo;
 wo.sync = sync_commit;
 if (sync_commit && batch)
	 err(result_code_t::sError) << "Attempting to do commits during batch";
*/

namespace sofadb {
	class Database;
	class snapshot_t;

	class storage_t
	{
	public:
		virtual ~storage_t() {}

		virtual bool try_get(const jstring_t &key, jstring_t *res,
							 snapshot_t *snap=0)=0;
		virtual void put(const jstring_t &key, const jstring_t &val)=0;

		virtual snapshot_t* snapshot();
		virtual void release_snapshot(snapshot_t*);
	};

	class batch_storage_t : public storage_t
	{
	public:
		virtual void commit(bool sync) = 0;
	};

	typedef boost::shared_ptr<storage_t> storage_ptr_t;
	typedef boost::shared_ptr<batch_storage_t> batch_storage_ptr_t;

}; //namespace sofadb

#endif //STORAGE_INTERFACE
