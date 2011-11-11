#include "erlang_compat.h"
#include <boost/make_shared.hpp>
#include <vector>
#include <streambuf>
#include <assert.h>
#include <limits.h>

using namespace erlang;
using namespace utils;

list_ptr_t list_t::make()
{
	return boost::make_shared<list_t>();
}

list_ptr_t list_t::make(const list_t &other)
{
	auto res=boost::make_shared<list_t>();
	*res = other;
	return res;
}

struct term_to_binary_visitor : public boost::static_visitor<>
{
	output_stream *out;

	void operator()(const erl_nil_t &i) const
	{
		out->write_byte(NIL_EXT); //NIL_EXT
	}

	void operator()(const atom_t &i) const
	{
		out->write_byte(ATOM_EXT);
		const size_t sz=i.name_.size();
		assert(sz<=255);
		out->write_int2(sz);
		out->write(i.name_.data(), sz);
	}

	void operator()(const double &i) const
	{
		out->write_byte(FLOAT_EXT);
		char buf[31]={0};
		sprintf(buf, "%.20e", i);
		out->write(buf, 31);
	}

	void operator()(const integer_t &i) const
	{
	}

	void operator()(const std::string &str) const
	{
		if (str.size()>=MAX_STRING_LEN)
		{
			out->write_byte(LIST_EXT);
			assert(str.size()<UINT_MAX);
			out->write_int4(str.size());
			out->write(str.data(), str.size());
			out->write_byte(NIL_EXT);
		} else
		{
			out->write_byte(STRING_EXT);
			assert(str.size()<USHRT_MAX);
			out->write_int2(str.size());
			out->write(str.data(), str.size());
		}
	}

	void operator()(const list_ptr_t &ptr) const
	{
		assert(ptr);

		size_t list_size = 1;
		list_ptr_t cur=ptr;
		while(cur)
		{
			list_size++;
			cur=cur->next_;
		}

		out->write_byte(LIST_EXT);
		out->write_int4(list_size-1);

		cur=ptr;
		while(cur)
		{
			cur->val_.apply_visitor(*this);
			cur=cur->next_;
		}
	}

	void operator()(const tuple_ptr_t &ptr) const
	{
		if (ptr->elements_.size()<=0xFF)
		{
			out->write_byte(SMALL_TUPLE_EXT);
			out->write_byte(
						static_cast<unsigned char>(ptr->elements_.size()));
		} else
		{
			out->write_byte(LARGE_TUPLE_EXT);
			out->write_int4(static_cast<uint32_t>(ptr->elements_.size()));
		}
		for(auto iter=ptr->elements_.begin();
			iter!=ptr->elements_.end(); ++iter)
			iter->apply_visitor(*this);
	}
};

void erlang::term_to_binary(const erl_type_t &term, utils::output_stream *out)
{
	out->write_byte(VERSION_MAGIC);
	term_to_binary_visitor visitor;
	visitor.out = out;
	term.apply_visitor(visitor);
}

int test()
{
	erl_type_t tst;
	tst = std::string("Hello, world");
}
