#include "erlang_compat.h"
#include <boost/make_shared.hpp>
#include <boost/cast.hpp>
#include <vector>
#include <streambuf>
#include <assert.h>
#include <limits.h>

using boost::numeric_cast;
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
		if (sz>255)
			throw std::out_of_range("Atom is too long: "+
									i.name_.substr(0,20)+"...");
		out->write_uint2(sz);
		out->write(i.name_.data(), sz);
	}

	void operator()(const double &i) const
	{
		out->write_byte(FLOAT_EXT);
		char buf[31]={0};
		sprintf(buf, "%.20e", i);
		out->write(buf, 31);
	}

	void operator()(const BigInteger &i) const
	{
		if (i<=UCHAR_MAX && i>=0)
		{
			out->write_byte(SMALL_INTEGER_EXT);
			out->write_byte(i.toUnsignedChar());
			return;
		}

		if (i>=ERLANG_INT_MIN && i<=ERLANG_INT_MAX)
		{
			out->write_byte(INTEGER_EXT);
			out->write_sint4(i.toInt());
			return;
		}

		unsigned int bits=i.getMagnitude().bitLength();
		if (bits>=INT_MAX)
			throw std::out_of_range("Big number is way too big");
		unsigned int bytes = (bits+7)/8; //No overflow is possible here
		assert(bytes>0); //Zero is handled earlier

		if (bytes<=UCHAR_MAX)
		{
			out->write_byte(SMALL_BIG_EXT);
			out->write_byte(bytes);
			out->write_byte(i.getSign()==BigInteger::negative ? 1:0);
		} else
		{
			out->write_byte(LARGE_BIG_EXT);
			out->write_uint4(bytes);
			out->write_byte(i.getSign()==BigInteger::negative ? 1:0);
		}
		//Output data
		BigUnsigned magnitude=i.getMagnitude();
		while(magnitude>0)
		{
			unsigned char cur_val=(magnitude%256).toInt();
			out->write_byte(cur_val);
			magnitude >>= 8;
		}
	}

	void operator()(const std::string &str) const
	{
		if (str.size()<=MAX_STRING_LEN)
		{
			out->write_byte(STRING_EXT);
			assert(str.size()<USHRT_MAX);
			out->write_uint2(str.size());
			out->write(str.data(), str.size());
		} else
		{
			out->write_byte(LIST_EXT);
			out->write_uint4(str.size());
			for(auto iter=str.begin();iter!=str.end();++iter)
			{
				erl_type_t val=BigInteger(*iter);
				val.apply_visitor(*this);
			}
			out->write_byte(NIL_EXT);
		}
	}

	void operator()(const list_ptr_t &ptr) const
	{
		assert(ptr);

		size_t list_size = 0;
		list_ptr_t cur=ptr;
		while(cur)
		{
			list_size++;
			cur=cur->next_;
		}

		out->write_byte(LIST_EXT);
		//Do not count the last element - it's always present and is treated
		//specially in the wire format.
		out->write_uint4(numeric_cast<uint32_t>(list_size-1));

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
			out->write_byte(numeric_cast<unsigned char>(ptr->elements_.size()));
		} else
		{
			out->write_byte(LARGE_TUPLE_EXT);
			out->write_uint4(numeric_cast<uint32_t>(ptr->elements_.size()));
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
