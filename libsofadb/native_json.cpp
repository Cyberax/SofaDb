#include "native_json.h"
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>
#include <boost/lexical_cast.hpp>
#include <BigIntegerUtils.hh>
#include "errors.h"

using namespace sofadb;

json_value json_value::normalize_int() const
{
	if (type()==int_d)
		return *this;
	else if (type()==big_int_d)
	{
		const BigInteger &bi=get_big_int();
		if (bi>=INT64_MIN && bi <= INT64_MAX)
			return json_value(int64_t(bi.toLong()));
		return *this;
	} else
		throw std::bad_cast();
}

bool sofadb::operator == (const json_value &l, const json_value &r)
{
	if (l.type()==big_int_d && r.type()==int_d ||
		r.type()==big_int_d && l.type()==int_d)
	{
		json_value l1=l.normalize_int();
		json_value l2=r.normalize_int();
		return l1.type()==l2.type() && l1.get_int() == l2.get_int();
	}

	if (l.type() != r.type())
		return false;
	switch(l.type())
	{
		case nil_d:
			return true;
		case bool_d:
			return l.get_bool() == r.get_bool();
		case int_d:
			return l.get_int() == r.get_int();
		case double_d:
			return l.get_double() == r.get_double();
		case string_d:
			return l.get_str() == r.get_str();
		case big_int_d:
			return l.get_big_int() == r.get_big_int();
		case submap_d:
			return l.get_submap() == r.get_submap();
		case sublist_d:
			return l.get_sublist() == r.get_sublist();
		default:
			assert(false);
	}
}

bool sofadb::operator < (const json_value &l, const json_value &r)
{
	if (l.type() != r.type())
		return false;
	switch(l.type())
	{
		case nil_d:
			return true;
		case bool_d:
			return l.get_bool() < r.get_bool();
		case int_d:
			return l.get_int() < r.get_int();
		case double_d:
			return l.get_double() < r.get_double();
		case string_d:
			return l.get_str() < r.get_str();
		case big_int_d:
			return l.get_big_int() < r.get_big_int();
		case submap_d:
			return l.get_submap() < r.get_submap();
		case sublist_d:
			return l.get_sublist() < r.get_sublist();
		default:
			assert(false);
	}
}

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
	std::vector<json_value*> stack_;
	result_code_t error_;
};

static json_value* advance_list_with(json_processor *proc, json_value && val)
{
	json_value* top=proc->stack_.back();
	if (top->is_sublist())
		top->get_sublist().push_back(std::move(val));
	else
	{
		(*top) = std::move(val);
		proc->stack_.pop_back();
	}
	return top;
}

static int json_null(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	advance_list_with(proc, json_value());
	return 1;
}

static int json_boolean(void * ctx, int boolVal)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	advance_list_with(proc, json_value((bool)boolVal));
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
		json_value v(stringToBigInteger(digits));
		advance_list_with(proc, v.normalize_int());
	}
	return 1;
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
	json_value *new_elem=advance_list_with(proc, json_value());
	new_elem->as_sublist();
	proc->stack_.push_back(new_elem);
	return 1;
}

static int json_end_array(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	assert(!proc->stack_.empty());
	proc->stack_.pop_back();
	return 1;
}

static int json_start_map(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	json_value *new_elem=advance_list_with(proc, json_value());
	new_elem->as_submap();
	proc->stack_.push_back(new_elem);
	return 1;
}

static int json_map_key(void * ctx, const unsigned char * key,
				 unsigned int stringLen)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	json_value *top = proc->stack_.back();
	assert(top->is_submap());

	json_value *cur_val=&top->as_submap()[
			std::string((const char*)key, stringLen)];
	proc->stack_.push_back(cur_val);

	return 1;
}

static int json_end_map(void * ctx)
{
	json_processor *proc = static_cast<json_processor*>(ctx);
	assert(!proc->stack_.empty());
	proc->stack_.pop_back();
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

json_value sofadb::string_to_json(const std::string &str)
{
	json_value res;
	res.as_submap();

	json_processor processor;
	processor.stack_.push_back(&res);

	boost::shared_ptr<yajl_handle_t> hndl(
				yajl_alloc(&json_callbacks, &config, &alloc_funcs, &processor),
				yajl_deleter());
	yajl_status status=yajl_parse(hndl.get(),
								  (const unsigned char*)str.c_str(),
								  str.size());
	handle_status(hndl.get(), status, str);
	yajl_status status2=yajl_parse_complete(hndl.get());
	handle_status(hndl.get(), status2, str);

	//assert(processor.stack_.size());
	return res;
}

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

struct json_printer
{
	boost::shared_ptr<yajl_gen_t> ptr_;

	void operator()()
	{
		check_status(yajl_gen_null(ptr_.get()));
	}
	void operator()(bool b)
	{
		check_status(yajl_gen_bool(ptr_.get(), b));
	}
	void operator()(int64_t i)
	{
		std::string stringy=boost::lexical_cast<std::string>(i);
		check_status(yajl_gen_number(ptr_.get(), stringy.data(),
									 stringy.size()));
	}
	void operator()(double d)
	{
		check_status(yajl_gen_double(ptr_.get(), d));
	}
	void operator()(const std::string &s)
	{
		check_status(yajl_gen_string(ptr_.get(),
									 (const unsigned char*)(s.data()),
									 s.size()));
	}
	void operator()(const BigInteger &b)
	{
		std::string num=bigIntegerToString(b);
		check_status(yajl_gen_number(ptr_.get(),
									 num.data(),
									 num.length()));
	}
	void operator()(const submap_t &map)
	{
		check_status(yajl_gen_map_open(ptr_.get()));
		for(auto i=map.begin(), iend=map.end();i!=iend;++i)
		{
			const std::string &k=i->first;
			check_status(yajl_gen_string(ptr_.get(),
										 (const unsigned char*)k.data(),
										 k.size()));
			i->second.apply_visitor(*this);
		}
		check_status(yajl_gen_map_close(ptr_.get()));
	}
	void operator()(const sublist_t &lst)
	{
		check_status(yajl_gen_array_open(ptr_.get()));
		for(const auto &el : lst)
			el.apply_visitor(*this);
		check_status(yajl_gen_array_close(ptr_.get()));
	}
};

std::string sofadb::json_to_string(const json_value &val, bool pretty)
{
	printer_context ctx;
	ctx.res_.reserve(128);
	yajl_gen_config json_gen_config = {pretty?1:0, " "};
	auto ptr=boost::shared_ptr<yajl_gen_t>(
				yajl_gen_alloc2(&json_print, &json_gen_config,
								&alloc_funcs, &ctx),
				printer_deleter());

	json_printer p;
	p.ptr_ = ptr;
	val.apply_visitor(p);

	yajl_gen_clear(ptr.get());
	return ctx.res_;
}
