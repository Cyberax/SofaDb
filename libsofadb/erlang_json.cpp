#include "erlang_compat.h"
#include "errors.h"
#include <yajl/yajl_parse.h>
#include <boost/utility/value_init.hpp>

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

	result_code_t error_;
};

void ensure_list_is_on_stack(json_processor *ctx)
{
	erl_type_t &tp = ctx->stack_.back();
	if(tp.type()!=typeid(list_ptr_t))
		abort();
}

void advance_list_with(json_processor *proc, erl_type_t && val)
{
	ensure_list_is_on_stack(proc);

	list_ptr_t &head=proc->heads_.back();
	head->val_=std::move(val);
	head->next_=list_t::make();
	head=head->next_;
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

static int json_start_map(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
}

static int json_map_key(void * ctx, const unsigned char * key,
				 unsigned int stringLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
}

static int json_end_map(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
}

static int json_start_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	ensure_list_is_on_stack(proc);

	list_ptr_t new_head=list_t::make();
	advance_list_with(proc, new_head);

	proc->stack_.push_back(new_head);
	proc->heads_.push_back(new_head);

	return 1;
}

static int json_end_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	ensure_list_is_on_stack(proc);
	proc->stack_.pop_back();
	proc->heads_.pop_back();

	return 1;
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

	return erl_nil_t();
}

std::string erlang::json_to_string(const erl_type_t &str)
{
	return "";
}
