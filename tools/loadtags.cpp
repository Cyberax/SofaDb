#include <stdio.h>
#include <fstream>
#include <iostream>
#include "database.h"
#include "engine.h"

using namespace sofadb;

int main(int argc, char **argv)
{
	if (argc!=3)
	{
		std::cerr << "Usage: loadtags <dumpfile> <database>" << std::endl;
		return 1;
	}

	jstring_t dump_file = argv[1];

	DbEngine engine(argv[2], false);
	database_ptr ptr=engine.create_a_database("tags");

	std::ifstream stream(dump_file);
	int ch=stream.get();
	if (stream.eof() || ch!='[')
	{
		std::cerr << "Premature end of file" << std::endl;
		return 2;
	}

	storage_ptr_t stg=engine.create_storage(false);

//	json_value val=json_from_stream(stream);
//	for(int f=0;f<20000;++f)
//		ptr->put(stg.get(), "test"+int_to_string(f),
//			 revision_num_t::empty_revision, json_value(), val);
//	return 0;
//	for(int f=0;f<1000;++f)
//	{
//		json_from_stream(stream);
//		char ch=stream.get();
//	}
//	return 0;

	while(true)
	{
		json_value val=json_from_stream(stream);
		ptr->put(stg.get(), val["metadata"]["track_id"].as_str(),
				 revision_num_t::empty_revision, val, false);

		char ch=stream.get();
		if (ch == ']')
			break;
		if (stream.eof() || ch!=',')
		{
			std::cerr << "Unexpected char " << ch << std::endl;
			return 2;
		}
	}

	return 0;
}
