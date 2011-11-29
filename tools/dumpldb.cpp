#include <leveldb/db.h>
#include <iostream>
#include <memory>

using namespace leveldb;

void print_pair(const Slice & key, const Slice &val)
{
	const static char alphabet[]="0123456789abcdef";

	std::cout << key.ToString() << " : ";
//	std::string offset;
//	offset.resize(key.ToString().length()+3, ' ');

	for(size_t f=0;f < val.size(); ++f)
	{
		const char cur=val[f];
		if (cur=='\n')
			std::cout << "\\n";
		else if (cur=='\r')
			std::cout << "\\r";
		else if (isprint(cur))
			std::cout << cur;
		else
		{
			std::cout << '\\' << alphabet[cur/16] << alphabet[cur%16];
		}

//		if (f!=0 && (f%70)==0 && f!=(val.size()-1))
//			std::cout << std::endl << "\t";
	}
	std::cout << std::endl;
}

int main(int argc, const char **argv)
{
	if (argc!=2)
	{
		std::cerr << "Usage: dumpldb <filename>" << std::endl;
		return 1;
	}

	Options opts;
	opts.block_size=1024;
	opts.create_if_missing = false;
	opts.paranoid_checks=1;
	opts.compression=kSnappyCompression;
	DB *db=0;
	leveldb::Status status = leveldb::DB::Open(opts, argv[1], &db);
	std::auto_ptr<DB> deleter(db);

	if (!status.ok())
	{
		std::cerr << "Failed to open DB " << status.ToString() << std::endl;
		return 1;
	}

	ReadOptions ro;
	std::auto_ptr<Iterator> iter(db->NewIterator(ro));

	iter->SeekToFirst();
	while(iter->Valid())
	{
		if (!iter->status().ok())
		{
			std::cerr << "Iteration error " << iter->status().ToString()
						<< std::endl;
			return 2;
		}

		print_pair(iter->key(), iter->value());
		iter->Next();
	}

	return 0;
}
