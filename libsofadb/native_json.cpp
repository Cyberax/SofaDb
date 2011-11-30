#include "native_json.h"
#include "errors.h"

#include "native_json_helpers.h"
#include "rapidjson/writer.h"
#include "rapidjson/reader.h"

#define BOOST_KARMA_NUMERICS_LOOP_UNROLL 6
#include <boost/spirit/include/karma.hpp>
using namespace boost::spirit;
using boost::spirit::karma::int_;
using boost::spirit::karma::lit;

using namespace sofadb;
using namespace utils;
using namespace rapidjson;

static const bignum_t max_int_decimal("9223372036854775807"); //2^63
static const bignum_t min_int_decimal("-9223372036854775808"); //-(2^63)-1
const json_value json_value::empty_val;

std::string sofadb::int_to_string(int64_t in)
{
	char buffer[64];
	char *ptr = buffer;
	karma::generate(ptr, int_, in);
	*ptr = '\0';
	return std::string(buffer, ptr-buffer);
}

bool sofadb::operator < (const bignum_t &l, const bignum_t &r)
{
	const bool lneg = l.digits_.at(0) == '-';
	const bool rneg = r.digits_.at(0) == '-';

	//Check for unequal signs
	if(lneg && !rneg)
		return true;
	if (!lneg && rneg)
		return false;

	//Signs are equal.
	if (l.digits_.size()!=r.digits_.size())
	{
		if (lneg)
			return l.digits_.size()>r.digits_.size();
		else
			return l.digits_.size()<r.digits_.size();
	}

	//Use STRING comparison to find out the relation of numbers
	//It's safe because the length of the left number is equal
	//to the length of the right number so lexiographical string
	//comparison (in all sane encodings) produces the same result
	//as numerical one.
	if (lneg)
		return l.digits_ > r.digits_;
	else
		return l.digits_ < r.digits_;
}

bool sofadb::operator < (const graft_t &l, const graft_t &r)
{
	return l.grafted() < r.grafted();
}

bool sofadb::operator == (const graft_t &l, const graft_t &r)
{
	return l.grafted() == r.grafted();
}

graft_t::graft_t() :
	graft_(&json_value::empty_val), deleter_()
{
}

json_value json_value::normalize_int() const
{
	if (type()==int_d)
		return *this;
	else if (type()==big_int_d)
	{
		const bignum_t &num = get_big_int();
		if ( (num>min_int_decimal || num==min_int_decimal) &&
			 (num<max_int_decimal || num==max_int_decimal) )
			 return json_value(int64_t(atoll(num.digits_.c_str())));

		return *this;
	} else
		throw std::bad_cast();
}

template<template<class T> class Operator> bool do_operation(
	const json_value &l, const json_value &r)
{
	if (l.type()==big_int_d && r.type()==int_d ||
		r.type()==big_int_d && l.type()==int_d)
	{
		const json_value &l1=l.normalize_int();
		const json_value &l2=r.normalize_int();
		if (l1.type()!=l2.type())
			return Operator<json_disc>()(l1.type(), l2.type());
		return Operator<int64_t>()(l1.get_int(), l2.get_int());
	}

	if (l.type()==graft_d && r.type()!=graft_d)
		return Operator<json_value>()(l.get_graft().grafted(), r);
	if (r.type()==graft_d && l.type()!=graft_d)
		return Operator<json_value>()(l, r.get_graft().grafted());

	if (l.type() != r.type())
		return Operator<json_disc>()(l.type(), r.type());
	switch(l.type())
	{
		case nil_d:
			return Operator<int>()(1, 1);
		case bool_d:
			return Operator<bool>()(l.get_bool(), r.get_bool());
		case int_d:
			return Operator<int64_t>()(l.get_int(), r.get_int());
		case double_d:
			return Operator<double>()(l.get_double(), r.get_double());
		case string_d:
			return Operator<jstring_t>()(l.get_str(), r.get_str());
		case big_int_d:
			return Operator<bignum_t>()(l.get_big_int(), r.get_big_int());
		case submap_d:
			return Operator<submap_t>()(l.get_submap(), r.get_submap());
		case sublist_d:
			return Operator<sublist_t>()(l.get_sublist(), r.get_sublist());
		case graft_d:
			return Operator<graft_t>()(l.get_graft(), r.get_graft());
		default:
			assert(false);
	}
}

bool sofadb::operator == (const json_value &l, const json_value &r)
{
	return do_operation<std::equal_to>(l, r);
}

bool sofadb::operator < (const json_value &l, const json_value &r)
{
	return do_operation<std::less>(l, r);
}


struct rapid_read_handler
{
	std::vector<json_value*> values_;
	jstring_t cur_key_;
	bool first_;
	rapid_read_handler() : first_(true) {}

	json_value* advance(json_value && val)
	{
		json_value &cur_tip = *values_.back();
		if (!cur_key_.empty())
		{
			auto res=cur_tip.get_submap().insert(
						std::make_pair(std::move(cur_key_),
									   std::move(val)));
			cur_key_.clear();
			return &res.first->second;
		} else
		{
			cur_tip.get_sublist().push_back(std::move(val));
			return &cur_tip.get_sublist().back();
		}
	}

	void Null()
	{
		advance(json_value());
	}
	void Bool(bool b)
	{
		advance(json_value(b));
	}

	void BigNum(const char *str, size_t length)
	{
		std::string digits(str, length);
		if (digits.find_first_of("eE.")!=jstring_t::npos)
		{
			double val=0;
			if (sscanf(digits.c_str(), "%lf", &val) == EOF)
				throw std::bad_exception();
			advance(json_value(val));
		} else
		{
			json_value v(bignum_t(std::move(digits)));
			advance(v.normalize_int());
		}
	}

	void String(const char* str, size_t length, bool copy)
	{
		json_value &cur_val = *values_.back();
		if (cur_key_.empty() && cur_val.type()==submap_d)
		{
			cur_key_ = jstring_t(str, length);
		} else
		{
			advance(json_value(jstring_t(str, length)));
		}
	}

	void StartObject()
	{
		if (first_)
		{
			first_ = false;
			values_.back()->as_submap();
			return;
		}
		json_value *val=advance(json_value(submap_d));
		values_.push_back(val);
	}
	void EndObject(SizeType memberCount)
	{
		values_.pop_back();
	}
	void StartArray()
	{
		if (first_)
		{
			first_ = false;
			values_.back()->as_sublist();
			return;
		}
		json_value *val=advance(json_value(sublist_d));
		values_.push_back(val);
	}
	void EndArray(SizeType elementCount)
	{
		values_.pop_back();
	}
};

template<class Stream> json_value parse_from_stream(Stream &istr)
{
	char alloc_buf[8192];
	MemoryPoolAllocator<> alloc(alloc_buf, 8192);
	Reader reader(&alloc);

	json_value res;
	rapid_read_handler hndl;
	hndl.values_.push_back(&res);

	reader.Parse<0>(istr, hndl);

	return std::move(res);
}

json_value sofadb::string_to_json(const jstring_t &str)
{
	BufReadStream istr((char*)str.data(), str.size());
	return parse_from_stream(istr);
}

json_value sofadb::json_from_stream(std::istream &val)
{
	StdStreamReadStream istr(val);
	return parse_from_stream(istr);
}

struct rapid_json_printer
{
	Writer<StringWriteStream> &writer_;
	rapid_json_printer(Writer<StringWriteStream> &writer) : writer_(writer) {}

	void operator()()
	{
		writer_.Null();
	}
	void operator()(bool b)
	{
		writer_.Bool(b);
	}
	void operator()(int64_t i)
	{
		writer_.Int64(i);
	}
	void operator()(double d)
	{
		writer_.Double(d);
	}
	void operator()(const jstring_t &s)
	{
		writer_.String(s.data(), s.length());
	}
	void operator()(const bignum_t &b)
	{
		writer_.BigInt(b.digits_.data(), b.digits_.length());
	}
	void operator()(const submap_t &map)
	{
		writer_.StartObject();
		for(auto i=map.begin(), iend=map.end();i!=iend;++i)
		{
			writer_.String(i->first.data(), i->first.length());
			i->second.apply_visitor(*this);
		}
		writer_.EndObject();
	}
	void operator()(const sublist_t &lst)
	{
		writer_.StartArray();
		for(auto i = lst.begin(), iend=lst.end(); i!=iend; ++i)
			i->apply_visitor(*this);
		writer_.EndArray();
	}
	void operator()(const graft_t &lst)
	{
		lst.grafted().apply_visitor(*this);
	}
};

void sofadb::json_to_string(jstring_t &append_to,
							const json_value &val, bool pretty)
{
	char buf[8192];
	MemoryPoolAllocator<> alloc(buf, 8192);
	StringWriteStream stream(append_to);
	Writer<StringWriteStream> writer(stream, &alloc);
	rapid_json_printer vis(writer);
	val.apply_visitor(vis);
	alloc.Clear();
}
