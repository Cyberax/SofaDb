#include "errors.h"
#include <sstream>

using namespace sofadb;

extern SOFADB_PUBLIC const die_t sofadb::die=die_t();
extern SOFADB_PUBLIC const result_code_t sofadb::sok=result_code_t();

sofa_exception::sofa_exception(const result_code_t &code) : code_(code)
{
	std::stringstream s;
	s<<("Error code: ")<<code.code()<<", description: "<<code.desc();
	what_=s.str();
};
