#include "erlang_compat.h"
#include <boost/make_shared.hpp>
#include <boost/cast.hpp>
#include <vector>
#include <streambuf>
#include <assert.h>
#include <limits.h>
#include <tuple>

using boost::numeric_cast;
using namespace erlang;
using namespace utils;

static erl_type_t read_term(utils::input_stream *in);

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

struct SOFADB_LOCAL term_to_binary_visitor : public boost::static_visitor<>
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

static BigInteger read_big_int(utils::input_stream *in, uint32_t ln)
{
	uint8_t sign=in->read_byte();
	BigUnsigned magnitude;

	std::vector<unsigned char> buf;
	buf.resize(ln);
	in->read(&buf[0], ln);

	for(auto iter=buf.rbegin(); iter!=buf.rend(); ++iter)
	{
		uint8_t byte=*iter;
		magnitude<<=8;
		magnitude|=byte;
	}
	return BigInteger(magnitude,
					  sign ? BigInteger::negative : BigInteger::positive);
}

static erl_type_t read_tuple(utils::input_stream *in, uint32_t ln)
{
	tuple_ptr_t res(new tuple_t());
	res->elements_.reserve(ln+1);
	for(uint32_t f=0;f<ln;++f)
		res->elements_.push_back(read_term(in));
	return res;
}

static std::tuple<list_ptr_t, list_ptr_t> unwind_string(const std::string & str)
{
	auto list_res=list_t::make();
	auto list_head=list_res;
	for(uint32_t pos=0; pos<str.length(); ++pos)
	{
		unsigned char ch=str[pos];
		list_res->val_=BigInteger(ch);
		list_res->next_=list_t::make();
		list_res=list_res->next_;
	}
	return std::make_tuple(list_head, list_res);
}

static erl_type_t read_list(utils::input_stream *in)
{
	uint32_t ln=in->read_uint4();

	std::string str_res;
	list_ptr_t list_res, list_head;
	bool accumulating_string=true;

	for(uint32_t f=0;f<ln;++f)
	{
		erl_type_t tp=read_term(in);
		const BigInteger *val=boost::get<BigInteger>(&tp);
		bool is_char=val && (*val)>=0 && (*val)<=UCHAR_MAX;

		if (is_char && accumulating_string)
		{
			str_res.push_back(val->toUnsignedChar());
			continue;
		}

		if (!is_char && accumulating_string)
		{
			//Unwind the string into list
			accumulating_string=false;
			auto res=unwind_string(str_res);
			list_head=std::get<0>(res);
			list_res=std::get<1>(res);
		}

		list_res->val_=tp;
		list_res->next_=list_t::make();
		list_res=list_res->next_;
	}

	erl_type_t tp=read_term(in); //tail
	if (accumulating_string)
	{
		if (tp.type()!=typeid(erl_nil_t))
		{
			//The tail is not of type NIL, so we have to unwind the string
			accumulating_string=false;
			auto res=unwind_string(str_res);
			list_head=std::get<0>(res);
			list_res=std::get<1>(res);
			list_res->val_=tp;
		}
	} else
		list_res->val_=tp;

	if (accumulating_string)
		return str_res;
	else
		return list_head;
}

static erl_type_t read_term(utils::input_stream *in)
{
	uint8_t tag=in->read_byte();
	switch(tag)
	{
	case NIL_EXT:
		return erl_nil_t();
	case ATOM_EXT:
		{
			uint16_t ln=in->read_uint2();
			if (ln>255)
				throw std::out_of_range("Atom name is too long");
			std::string name;
			name.resize(ln);
			in->read(&name[0], ln);

			atom_t res={name};
			return res;
		}
	case FLOAT_EXT:
		{
			char buf[32]={0};
			in->read(buf, 31);
			double res=0.0;
			sscanf(buf, "%lf", &res);
			return res;
		}
	case SMALL_INTEGER_EXT:
		{
			uint8_t i=in->read_byte();
			return BigInteger(i);
		}
	case INTEGER_EXT:
		{
			int32_t i=in->read_sint4();
			return BigInteger(i);
		}
	case SMALL_BIG_EXT:
		{
			uint8_t ln=in->read_byte();
			return read_big_int(in, ln);
		}
	case LARGE_BIG_EXT:
		{
			uint32_t ln=in->read_uint4();
			if (ln>INT_MAX/8)
				throw std::out_of_range("A number is too big");
			return read_big_int(in, ln);
		}
	case STRING_EXT:
		{
			uint16_t ln=in->read_uint2();
			std::string str;
			str.resize(ln);
			in->read(&str[0], ln);
			return str;
		}
	case SMALL_TUPLE_EXT:
		{
			uint8_t ln=in->read_byte();
			return read_tuple(in, ln);
		}
	case LARGE_TUPLE_EXT:
		{
			uint32_t ln=in->read_uint4();
			return read_tuple(in, ln);
		}
	case LIST_EXT:
		{
			return read_list(in);
		}

	default:
		throw std::logic_error("Unknown tag");
	}
}

erl_type_t erlang::binary_to_term(utils::input_stream *in)
{
	uint8_t magic=in->read_byte();
	if (magic!=VERSION_MAGIC)
		throw std::logic_error("Incorrect version magic");

	return read_term(in);
}

struct SOFADB_LOCAL equality_visitor : public boost::static_visitor<>
{
	const erl_type_t &r_;
	bool res_;

	equality_visitor(const erl_type_t &r) : r_(r), res_(false) {}

	void operator()(const erl_nil_t &i)
	{
		res_=true;
	}

	void operator()(const atom_t &i)
	{
		res_=i.name_==boost::get<atom_t>(r_).name_;
	}

	void operator()(const double &i)
	{
		//Aside: astute readers might have noticed that we're
		//comparing two doubles here using == operator which is generally
		//a big NO-NO. However, in this case we are interested in bit-perfect
		//compatibility.
		res_=i==boost::get<double>(r_);
	}

	void operator()(const BigInteger &i)
	{
		res_=i==boost::get<BigInteger>(r_);
	}

	void operator()(const std::string &str)
	{
		res_=str==boost::get<std::string>(r_);
	}

	void operator()(const list_ptr_t &ptr)
	{
		list_ptr_t cur=ptr;
		list_ptr_t cur_other=boost::get<list_ptr_t>(r_);
		while(true)
		{
			bool first_empty=!!cur;
			bool second_empty=!!cur_other;
			if (first_empty != second_empty)
				return;

			if (first_empty)
			{
				res_=true;
				return;
			}

			if (deep_ne(cur->val_,cur_other->val_))
				return;
			cur=cur->next_;
			cur_other=cur_other->next_;
		}
	}

	void operator()(const tuple_ptr_t &l_tuple)
	{
		tuple_ptr_t r_tuple=boost::get<tuple_ptr_t>(r_);
		if (r_tuple->elements_.size()!=l_tuple->elements_.size())
			return;

		for(size_t f=0;f<r_tuple->elements_.size();++f)
		{
			if (deep_ne(r_tuple->elements_.at(f),
						l_tuple->elements_.at(f)))
				return;
		}

		res_=true;
	}
};

bool erlang::deep_eq(const erl_type_t &l, const erl_type_t &r)
{
	if (l.which() != r.which())
		return false;

	equality_visitor vis(r);
	l.apply_visitor(vis);
	return vis.res_;
}
