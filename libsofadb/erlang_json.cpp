#include "erlang_compat.h"
#include "errors.h"
#include <yajl/yajl_parse.h>
#include <boost/utility/value_init.hpp>
#include <iostream>

using namespace erlang;
using namespace sofadb;

static void *yajl_malloc_impl(void *ctx, unsigned int sz)
{
	return malloc(sz);
}

static void yajl_free_impl(void *ctx, void * ptr)
{
	free(ptr);
}

static void * yajl_realloc_impl(void *ctx, void * ptr, unsigned int sz)
{
	return realloc(ptr, sz);
}

static yajl_alloc_funcs funcs = {
	&yajl_malloc_impl,
	&yajl_realloc_impl,
	&yajl_free_impl,
	0
};

static yajl_parser_config config = {1, 0};

struct yajl_deleter
{
	void operator ()(yajl_handle hndl)
	{
		yajl_free(hndl);
	}
};

struct json_processor
{
	std::vector<erl_type_t> stack_;
	std::vector<list_ptr_t> heads_;

	boost::value_initialized<bool> head_is_visited_;

	result_code_t error_;
};


void advance_list_with(json_processor *proc, erl_type_t && val)
{
	assert(!proc->stack_.empty());

	erl_type_t &tp=proc->stack_.back();
	if (tp.type()==typeid(tuple_ptr_t))
	{
		//This is the second element of the tuple
		boost::get<tuple_ptr_t>(tp)->elements_.push_back(std::move(val));
		proc->stack_.pop_back(); //We're done with the current tuple
	} else if (tp.type()==typeid(list_ptr_t))
	{
		assert(!proc->heads_.empty());

		list_ptr_t &tail=proc->heads_.back();
		if (proc->head_is_visited_.data())
		{
			tail->next_=list_t::make();
			tail=tail->next_;
		} else
			proc->head_is_visited_.data() = true;
		tail->val_=std::move(val);
	} else
		abort();
}

static int json_null(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	advance_list_with(proc, erl_nil_t());
	return 1;
}

static int json_boolean(void * ctx, int boolVal)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	atom_t atom;
	atom.name_ = boolVal? "true" : "false";
	advance_list_with(proc, std::move(atom));

	return 1;
}

/** A callback which passes the string representation of the number
 *  back to the client.  Will be used for all numbers when present */
static int json_number(void * ctx, const char * numberVal,
					unsigned int numberLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	//erl_type_t digits(std::string((const char*)numberVal, numberLen));
	//stringToBigInteger(sd)
}

/** strings are returned as pointers into the JSON text when,
 * possible, as a result, they are _not_ null padded */
static int json_string(void * ctx, const unsigned char * stringVal,
					unsigned int stringLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	advance_list_with(proc, std::string((const char*)stringVal, stringLen));

	return 1;
}

static int json_start_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);

	list_ptr_t new_head=list_t::make();
	advance_list_with(proc, new_head);

	proc->stack_.push_back(new_head);
	proc->heads_.push_back(new_head);
	proc->head_is_visited_.data() = false;

	return 1;
}

static int json_end_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);

	assert(!proc->stack_.empty() && !proc->heads_.empty());
	proc->stack_.pop_back();
	proc->heads_.pop_back();
	assert(proc->head_is_visited_.data());

	return 1;
}

static int json_start_map(void * ctx)
{
	return json_start_array(ctx);
}

static int json_map_key(void * ctx, const unsigned char * key,
				 unsigned int stringLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	const tuple_ptr_t map_entry(new tuple_t());
	advance_list_with(proc, map_entry);
	proc->stack_.push_back(map_entry);

	erl_type_t map_key(std::string((const char*)key, stringLen));
	map_entry->elements_.push_back(map_key);

	return 1;
}

static int json_end_map(void * ctx)
{
	return json_end_array(ctx);
}

static yajl_callbacks json_callbacks = {
	&json_null,
	&json_boolean,
	0, //json_integer
	0, //yajl_double
	&json_number,
	&json_string,
	&json_start_map,
	&json_map_key,
	&json_end_map,
	&json_start_array,
	&json_end_array,
};

void handle_status(yajl_handle hndl, yajl_status status, const std::string &str)
{
	if (status==yajl_status_ok)
		return;

	unsigned char* err=yajl_get_error(hndl, 1,
		(const unsigned char*)str.c_str(), str.size());
	result_code_t res(result_code_t::sError, (const char*)err);
	yajl_free_error(hndl, err);

	boost::throw_exception(sofa_exception(res));
}

erl_type_t erlang::parse_json(const std::string &str)
{
	json_processor processor;
	list_ptr_t head=list_t::make();
	processor.stack_.push_back(erl_type_t(head));
	processor.heads_.push_back(head);

	boost::shared_ptr<yajl_handle_t> hndl(
				yajl_alloc(&json_callbacks, &config, &funcs, &processor),
				yajl_deleter());
	yajl_status status=yajl_parse(hndl.get(),
								  (const unsigned char*)str.c_str(),
								  str.size());
	handle_status(hndl.get(), status, str);
	yajl_status status2=yajl_parse_complete(hndl.get());
	handle_status(hndl.get(), status2, str);

	std::cout << head->val_ << std::endl;
	return erl_nil_t();
}

std::string erlang::json_to_string(const erl_type_t &str)
{
	return "";
}
