#ifndef ERLANG_JSON
#define ERLANG_JSON

#include "erlang_compat.h"

namespace erlang
{
	//////////////////JSON-related functionality/////////////
	SOFADB_PUBLIC erl_type_t parse_json(const std::string &str);
	SOFADB_PUBLIC std::string json_to_string(const erl_type_t &str,
											 bool pretty=true);

	SOFADB_PUBLIC const erl_type_t& get_val(const erl_type_t &tp,
											const std::string &str);
	SOFADB_PUBLIC bool has_key(const erl_type_t& tp, const std::string &str);

	SOFADB_PUBLIC erl_type_t& put_val(erl_type_t& tp, const std::string &str);
	SOFADB_PUBLIC erl_type_t create_submap();
	SOFADB_PUBLIC list_ptr_t get_json_list(const erl_type_t& tp);
};

#endif // ERLANG_JSON
