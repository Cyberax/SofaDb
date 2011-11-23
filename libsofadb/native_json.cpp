#include "native_json.h"

using namespace sofadb;

struct output_visitor
{
	std::ostream &str_;
	output_visitor(std::ostream &str) : str_(str) {}

	int operator()()
	{
	}

	int operator()(bool val)
	{
		return 1;
	}

	template<class T> int operator()(const T &v)
	{

	}
};

std::ostream& operator << (std::ostream &str, const json_value &val)
{
	output_visitor vis(str);
	val.apply_visitor(vis);
	return str;
}

void test()
{
	json_value v;

}
