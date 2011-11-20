#include "erlang_compat.h"
#include "errors.h"
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <boost/utility/value_init.hpp>
#include <iostream>
#include <BigIntegerUtils.hh>
#include <tuple>

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

static yajl_alloc_funcs alloc_funcs = {
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
	//Structure is: <current_tuple, current_list_tail, parent_ref>
	typedef std::tuple<erl_type_t, list_ptr_t, erl_type_t*> ent_t;
	std::vector<ent_t> stack_;

	result_code_t error_;
};

static erl_type_t* advance_list_with(json_processor *proc, erl_type_t && val)
{
	assert(!proc->stack_.empty());

	json_processor::ent_t &state = proc->stack_.back();
	erl_type_t &tp = std::get<0>(state);

	if (tp.type()==typeid(tuple_ptr_t))
	{
		//This is the second element of the tuple
		tuple_ptr_t tp_ptr=boost::get<tuple_ptr_t>(tp);
		tp_ptr->elements_.push_back(std::move(val));
		proc->stack_.pop_back(); //We're done with the tuple

		return &tp_ptr->elements_.back();
	} else if (tp.type()==typeid(list_ptr_t))
	{
		list_ptr_t &tail=std::get<1>(state);
		if (tail)
		{
			tail->next_=list_t::make();
			tail=tail->next_;
		} else
			tail = boost::get<list_ptr_t>(tp);
		tail->val_=std::move(val);
		return &tail->val_;
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
	std::string digits((const char*)numberVal, numberLen);
	if (digits.find_first_of("eE.")!=std::string::npos)
	{
		double val=0;
		if (sscanf(digits.c_str(), "%lf", &val) == EOF)
			return 0;
		advance_list_with(proc, val);
	} else
	{
		advance_list_with(proc, stringToBigInteger(digits));
	}
	return 1;
}

/** strings are returned as pointers into the JSON text when,
 * possible, as a result, they are _not_ null padded */
static int json_string(void * ctx, const unsigned char * stringVal,
					unsigned int stringLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	binary_ptr_t bin=binary_t::make_from_buf(stringVal, stringLen);
	advance_list_with(proc, bin);

	return 1;
}

static int json_start_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);

	list_ptr_t new_head=list_t::make();
	erl_type_t *new_elem=advance_list_with(proc, new_head);

	proc->stack_.push_back(std::make_tuple(erl_type_t(new_head),
										   list_ptr_t(),new_elem));

	return 1;
}

static int json_end_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);

	assert(!proc->stack_.empty());

	json_processor::ent_t ent=proc->stack_.back();
	if (std::get<1>(ent)==list_ptr_t())
	{
		//We've got an empty list, we can get it for JSONs like this:
		//{"a" : {}}
		//so we need to back off and replace it with erl_nil_t()
		*(std::get<2>(ent)) = erl_nil_t();
	}

	proc->stack_.pop_back();

	return 1;
}

static int json_start_map(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	const tuple_ptr_t map_start=tuple_t::make();
	erl_type_t *map_elem=advance_list_with(proc, map_start);

	proc->stack_.push_back(std::make_tuple(map_start, list_ptr_t(),
										   map_elem));

	return json_start_array(ctx);
}

static int json_map_key(void * ctx, const unsigned char * key,
				 unsigned int stringLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	const tuple_ptr_t map_entry=tuple_t::make();
	erl_type_t *map_elem=advance_list_with(proc, map_entry);
	proc->stack_.push_back(std::make_tuple(map_entry, list_ptr_t(), map_elem));

	binary_ptr_t bin=binary_t::make_from_buf(key, stringLen);
	map_entry->elements_.push_back(bin);

	return 1;
}

static int json_end_map(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);

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

static void handle_status(yajl_handle hndl, yajl_status status,
						  const std::string &str)
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
	erl_type_t head_tp(head);
	processor.stack_.push_back(std::make_tuple(head_tp, list_ptr_t(), &head_tp));

	boost::shared_ptr<yajl_handle_t> hndl(
				yajl_alloc(&json_callbacks, &config, &alloc_funcs, &processor),
				yajl_deleter());
	yajl_status status=yajl_parse(hndl.get(),
								  (const unsigned char*)str.c_str(),
								  str.size());
	handle_status(hndl.get(), status, str);
	yajl_status status2=yajl_parse_complete(hndl.get());
	handle_status(hndl.get(), status2, str);

	assert(processor.stack_.size());
	return head->val_;
}

static yajl_gen_config json_get_config = {1, "    "};

struct printer_context
{
	std::string res_;
};

struct SOFADB_LOCAL printer_deleter
{
	void operator ()(yajl_gen hndl)
	{
		yajl_gen_free(hndl);
	}
};

static void json_print(void * ctx, const char * str, unsigned int len)
{
	printer_context *prn = static_cast<printer_context*>(ctx);
	prn->res_.append(str, len);
}

static void check_status(yajl_gen_status st)
{
	if (st!=yajl_gen_status_ok)
		err(result_code_t::sError) << "Unepxected return code "<<
									  st<<" from the JSON printer";
}

static void print_map(boost::shared_ptr<yajl_gen_t> ptr, const erl_type_t &js);

static void print_element(boost::shared_ptr<yajl_gen_t> ptr,
						  const erl_type_t &tp)
{
	if (tp.type()==typeid(list_ptr_t))
	{
		//Array
		check_status(yajl_gen_array_open(ptr.get()));
		for(list_ptr_t cur=boost::get<list_ptr_t>(tp); !!cur; cur=cur->next_)
		{
			print_element(ptr, cur->val_);
		}
		check_status(yajl_gen_array_close(ptr.get()));
	} else if (tp.type()==typeid(tuple_ptr_t))
	{
		//Submap
		print_map(ptr, tp);
	} else if (tp.type()==typeid(binary_ptr_t))
	{
		binary_ptr_t cur = boost::get<binary_ptr_t>(tp);
		yajl_gen_string(ptr.get(), cur->binary_.data(), cur->binary_.size());
	} else if (tp.type()==typeid(BigInteger))
	{
		//Int
		std::string num=bigIntegerToString(boost::get<BigInteger>(tp));
		yajl_gen_number(ptr.get(), num.c_str(), num.length());
	} else if (tp.type()==typeid(atom_t))
	{
		//Boolean
		const std::string &str=boost::get<const atom_t&>(tp).name_;
		if (str=="true")
			yajl_gen_bool(ptr.get(), 1);
		else if (str=="false")
			yajl_gen_bool(ptr.get(), 0);
		else
			err(result_code_t::sWrongRevision) << "Malformed boolean " << tp;
	} else if (tp.type()==typeid(double))
	{
		char buf[32] = {0};
		sprintf(buf, "%f", boost::get<double>(tp));
		yajl_gen_number(ptr.get(), buf, strlen(buf));
	} else if (tp.type()==typeid(erl_nil_t))
	{
		//Null
		yajl_gen_null(ptr.get());
	} else
		err(result_code_t::sWrongRevision) << "Unexpected JSON term "<<tp;
}

static void print_map(boost::shared_ptr<yajl_gen_t> ptr, const erl_type_t &js)
{
	if (js.type()!=typeid(tuple_ptr_t))
		err(result_code_t::sWrongRevision) << "Not a JSON document";
	tuple_ptr_t cur = boost::get<tuple_ptr_t>(js);
	if (cur->elements_.size()!=1)
		err(result_code_t::sError) << "Malformed JSON";
	erl_type_t sub = cur->elements_.at(0);

	if (sub.type()==typeid(erl_nil_t))
	{
		check_status(yajl_gen_map_open(ptr.get()));
		check_status(yajl_gen_map_close(ptr.get()));
		return;
	}

	if (sub.type()!=typeid(list_ptr_t))
		err(result_code_t::sWrongRevision) << "Document is not well formed";
	const list_ptr_t &lst=boost::get<list_ptr_t>(sub);

	check_status(yajl_gen_map_open(ptr.get()));
	for(list_ptr_t cur=lst; !!cur; cur=cur->next_)
	{
		//This list should contain only tuples
		if (cur->val_.type()!=typeid(tuple_ptr_t))
			err(result_code_t::sError) << "Unexpected JSON structure, "
										  "got "<<cur->val_.type().name()<<
										  " while expecting a tuple";
		const tuple_ptr_t &tp=boost::get<tuple_ptr_t>(cur->val_);
		if (tp->elements_.size()!=2)
			err(result_code_t::sError) << "Unexpected JSON structure, "
										  "got "<<tp->elements_.size()<<
										  " elements instead of 2";

		print_element(ptr, tp->elements_.at(0));
		print_element(ptr, tp->elements_.at(1));
	}
	check_status(yajl_gen_map_close(ptr.get()));
}

std::string erlang::json_to_string(const erl_type_t &js)
{
	printer_context ctx;
	auto ptr=boost::shared_ptr<yajl_gen_t>(
				yajl_gen_alloc2(&json_print, &json_get_config,
								&alloc_funcs, &ctx),
				printer_deleter());

	print_map(ptr, js);

	yajl_gen_clear(ptr.get());
	return ctx.res_;
}

std::string erlang::binary_to_string(const erl_type_t &tp)
{
	binary_ptr_t ptr=boost::get<binary_ptr_t>(tp);
	return std::string(ptr->binary_.begin(), ptr->binary_.end());
}

static bool find_key(const erl_type_t &tp,
					 const std::string &str, erl_type_t **out)
{
	erl_type_t str_bin=binary_t::make_from_string(str);

	tuple_ptr_t ptr=boost::get<tuple_ptr_t>(tp);
	assert(ptr->elements_.size()==1);

	const erl_type_t elem=ptr->elements_.at(0);
	if (elem.type()==typeid(erl_nil_t))
		return false;

	list_ptr_t lst=boost::get<list_ptr_t>(elem);
	for(list_ptr_t cur=lst; !!cur; cur=cur->next_)
	{
		tuple_ptr_t tpl=boost::get<tuple_ptr_t>(cur->val_);
		assert(tpl->elements_.size()==2);

		if (deep_eq(tpl->elements_.at(0),str_bin))
		{
			if (out)
				*out = &tpl->elements_.at(1);
			return true;
		}
	}

	return false;
}

bool erlang::has_key(const erl_type_t &tp, const std::string &str)
{
	return find_key(tp, str, 0);
}

const erl_type_t& erlang::get_val(const erl_type_t &tp, const std::string &str)
{
	erl_type_t *res = 0;
	if (!find_key(tp, str, &res))
		throw boost::bad_get();
	return *res;
}

erl_type_t& erlang::put_val(erl_type_t& tp, const std::string &str)
{
	erl_type_t *out;
	if (find_key(tp, str, &out))
		return *out;

	tuple_ptr_t ptr=boost::get<tuple_ptr_t>(tp);
	assert(ptr->elements_.size()==1);
	erl_type_t &cur_elem = ptr->elements_.at(0);

	list_ptr_t head;

	//We have an empty list - transform it into a list
	if (cur_elem.type()==typeid(erl_nil_t))
	{
		head=list_t::make();
		cur_elem = head;
	} else
	{
		list_ptr_t tail=boost::get<list_ptr_t>(cur_elem);
		head=list_t::make();
		head->next_=tail;
		cur_elem = head;
	}

	tuple_ptr_t tpl=tuple_t::make();
	head->val_ = tpl;
	tpl->elements_.push_back(binary_t::make_from_string(str));
	tpl->elements_.push_back(erl_nil_t());

	return tpl->elements_.back();
}

erl_type_t erlang::create_submap()
{
	tuple_ptr_t tpl=tuple_t::make();
	tpl->elements_.push_back(erl_nil_t());
	return tpl;
}
