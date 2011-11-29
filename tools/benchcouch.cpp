#include <stdio.h>
#include <fstream>
#include <iostream>
#include <curl/curl.h>
#include "native_json.h"

using namespace sofadb;

int main(int argc, char **argv)
{
	if (argc!=3)
	{
		std::cerr << "Usage: benchcouch <dump> <database_url>" << std::endl;
		return 1;
	}

	jstring_t dump_file = argv[1];

	std::ifstream stream(dump_file);
	int ch=stream.get();
	if (stream.eof() || ch!='[')
	{
		std::cerr << "Premature end of file" << std::endl;
		return 2;
	}

	curl_global_init(CURL_GLOBAL_ALL);

	bool finish=false;
	while(!finish)
	{
		json_value list(sublist_d);
		list.get_sublist().reserve(1001);

		for(int f=0;f<1000;++f)
		{
			json_value val=json_from_stream(stream);
			val["_id"] = val["metadata"]["track_id"];
			list.get_sublist().push_back(std::move(val));

			char ch=stream.get();
			if (ch == ']')
			{
				finish=true;
				break;
			}
			if (stream.eof() || ch!=',')
			{
				std::cerr << "Unexpected char" << ch << std::endl;
				return 2;
			}
		}

		if (list.get_sublist().empty())
			break;

		json_value docz(submap_d);
		docz["all_or_nothing"] = json_value(true);
		docz["docs"] = graft_t(&list);
		jstring_t docval = json_to_string(docz);

		CURL *curl = curl_easy_init();
		//curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, argv[2]);

		//Set data
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, docval.data());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, docval.length());

		/* input file */
		//curl_easy_setopt(curl, CURLOPT_READDATA, fp);

		struct curl_slist *slist = NULL;
		slist = curl_slist_append(slist, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
		curl_easy_perform(curl);
		curl_slist_free_all(slist);
		curl_easy_cleanup(curl);
	}

	return 0;
}
