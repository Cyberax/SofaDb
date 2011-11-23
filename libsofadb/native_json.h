#ifndef NATIVE_JSON_H
#define NATIVE_JSON_H

#include "common.h"
#include <map>
#include <vector>
#include "BigInteger.hh"
#include <boost/interprocess/containers/string.hpp>

#define MAX_ALIGNED_MEM 16
#define SBO_LEN 32

namespace sofadb {

	typedef std::string jstring_t;

	class json_value;
	typedef std::map<jstring_t, json_value> submap_t;
	typedef std::vector<json_value> sublist_t;

	namespace detail {
		union storage_t
		{
			char max_aligned[MAX_ALIGNED_MEM];
			char storage1[sizeof(jstring_t)];
			char storage2[sizeof(int64_t)];
			char storage3[sizeof(double)];
			char storage4[sizeof(BigInteger)];
			char storage5[sizeof(double)];
			char storage6[sizeof(submap_t)];
			char storage7[sizeof(sublist_t)];
		};
	};

	enum json_disc {
		nil_d,
		bool_d,
		int_d,
		double_d,
		string_d,
		big_int_d,
		submap_d,
		sublist_d,
	};

	class json_value
	{
		json_disc disc_;
		detail::storage_t storage_;

#ifndef NDEBUG
		union {
			jstring_t *_str_val;
			int64_t *_int64_val;
			double *_double_val;
			BigInteger *_big_int_val;
			submap_t *_submap_val;
			sublist_t *_sublist_val;
		};
#endif
	public:
		json_value(json_disc type);
		json_value(const json_value& other);
		json_value() { disc_ = nil_d; }
		json_value(json_value &&);
		~json_value()
		{
			clear();
		}

		json_value& operator = (const json_value& other);
		json_value& operator = (json_value &&);

		json_disc type() const {return disc_;}
		json_value normalize_int() const;

		void as_nil() { clear(); }
		bool is_nil() { return disc_==nil_d; }

		const json_value& operator[](const std::string &str) const
		{
			return get_submap().at(str);
		}
		json_value& operator[](const std::string &str)
		{
			return get_submap()[str];
		}

#define STD_FUNCS(type, type_postfix, disc, is_explicit) \
		is_explicit json_value(const type &val) \
		{ \
			disc_ = nil_d; \
			make<type, disc>(val); \
		}\
		is_explicit json_value(type &&val) \
		{ \
			disc_ = nil_d; \
			make<type, disc>(std::move(val)); \
		}\
		bool is_##type_postfix() const { return disc_ == disc; } \
		const type& get_##type_postfix() const \
		{ \
			return get<type, disc>(); \
		} \
		type& get_##type_postfix()\
		{ \
			return get<type, disc>(); \
		} \
		type& as_##type_postfix() \
		{ \
			return as<type, disc>(); \
		} \
		void set_##type_postfix(const type &val) \
		{  \
			as<type, disc>() = val; \
		} \
		void set_##type_postfix(type &&val) \
		{ \
			clear(); \
			make<type, disc>(std::move(val)); \
		}

		STD_FUNCS(bool, bool, bool_d, explicit)
		STD_FUNCS(int64_t, int, int_d, explicit)
		STD_FUNCS(double, double, double_d, )
		STD_FUNCS(jstring_t, str, string_d, )
		STD_FUNCS(BigInteger, big_int, big_int_d,)
		STD_FUNCS(submap_t, submap, submap_d,)
		STD_FUNCS(sublist_t, sublist, sublist_d, )

		template<class Visitor> auto apply_visitor(
			Visitor &vis) -> decltype(vis());
		template<class Visitor> auto apply_visitor(
			Visitor &vis) const -> decltype(vis());
	private:
		void clear();
		template<class T, json_disc D> const T& get() const
		{
			if (disc_ != D) throw std::bad_cast();
			return *(reinterpret_cast<const T*>(storage_.max_aligned));
		}
		template<class T, json_disc D> T& get()
		{
			if (disc_ != D) throw std::bad_cast();
			return *(reinterpret_cast<T*>(storage_.max_aligned));
		}
		template<class T, json_disc D> T& as()
		{
			if (disc_ != D)
			{
				clear();
				make<T,D>();
			}
			return *(reinterpret_cast<T*>(storage_.max_aligned));
		}

		template<class T, json_disc D> void make()
		{
			assert(disc_ == nil_d);
			disc_ = D;
#ifndef NDEBUG
			_str_val = reinterpret_cast<jstring_t*>(storage_.max_aligned);
#endif
			new (storage_.max_aligned) T();
		}
		template<class T, json_disc D> void make(const T &ref)
		{
			assert(disc_ == nil_d);
			disc_ = D;
#ifndef NDEBUG
			_str_val = reinterpret_cast<jstring_t*>(storage_.max_aligned);
#endif
			new (storage_.max_aligned) T(ref);
		}
		template<class T, json_disc D> void make(T &&ref)
		{
			assert(disc_ == nil_d);
			disc_ = D;
#ifndef NDEBUG
			_str_val = reinterpret_cast<jstring_t*>(storage_.max_aligned);
#endif
			new (storage_.max_aligned) T(std::move(ref));
		}
	};

	SOFADB_PUBLIC std::string json_to_string(const json_value &val,
											 bool pretty = false);
	SOFADB_PUBLIC json_value string_to_json(const std::string &val);
	inline std::ostream& operator << (std::ostream &str, const json_value &val)
	{
		return str << json_to_string(val, false);
	}

	SOFADB_PUBLIC bool operator < (const json_value &l, const json_value &r);
	inline bool operator > (const json_value &l, const json_value &r)
	{
		return r<l;
	}
	SOFADB_PUBLIC bool operator == (const json_value &l, const json_value &r);
	inline bool operator != (const json_value &l, const json_value &r)
	{
		return !(l == r);
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	inline void json_value::clear()
	{
		switch(disc_)
		{
			case nil_d:
			case bool_d:
			case int_d:
			case double_d:
				break;
			case string_d:
				get_str().~jstring_t();
				break;
			case big_int_d:
				get_big_int().~BigInteger();
				break;
			case submap_d:
				get_submap().~submap_t();
				break;
			case sublist_d:
				get_sublist().~sublist_t();
				break;
			default:
				assert(false);
		}
		disc_ = nil_d;
	}

	inline json_value::json_value(json_disc type)
	{
		disc_ = nil_d;
		switch(type)
		{
			case nil_d:
				break;
			case bool_d:
				make<bool, bool_d>();
				break;
			case int_d:
				make<int, int_d>();
				break;
			case double_d:
				make<double, double_d>();
				break;
			case string_d:
				make<jstring_t, string_d>();
				break;
			case big_int_d:
				make<BigInteger, big_int_d>();
				break;
			case submap_d:
				make<submap_t, submap_d>();
				break;
			case sublist_d:
				make<sublist_t, sublist_d>();
				break;
			default:
				assert(false);
		}
	}

	inline json_value::json_value(const json_value& other)
	{
		disc_ = nil_d;
		*this = other;
	}

	inline json_value& json_value::operator = (const json_value& other)
	{
		if (&other == this)
			return *this;
		clear();

		if (other.disc_ == nil_d)
		{
			as_nil();
		} else if (other.disc_==bool_d)
		{
			make<bool, bool_d>(other.get_bool());
		} else if (other.disc_==int_d)
		{
			make<int64_t, int_d>(other.get_int());
		} else if (other.disc_==double_d)
		{
			make<double, double_d>(other.get_double());
		} else if (other.disc_==string_d)
		{
			make<jstring_t, string_d>(other.get_str());
		} else if (other.disc_==big_int_d)
		{
			make<BigInteger, big_int_d>(other.get_big_int());
		} else if (other.disc_==submap_d)
		{
			make<submap_t, submap_d>(other.get_submap());
		} else if (other.disc_==sublist_d)
		{
			make<sublist_t, sublist_d>(other.get_sublist());
		} else
			assert(false);

		return *this;
	}

	inline json_value& json_value::operator = (json_value&& other)
	{
		if (&other == this)
			return *this;
		clear();

		if (other.disc_ == nil_d)
		{
			as_nil();
		} else if (other.disc_==bool_d)
		{
			make<bool, bool_d>(other.get_bool());
		} else if (other.disc_==int_d)
		{
			make<int64_t, int_d>(other.get_int());
		} else if (other.disc_==double_d)
		{
			make<double, double_d>(other.get_double());
		} else if (other.disc_==string_d)
		{
			make<jstring_t, string_d>(std::move(other.get_str()));
		} else if (other.disc_==big_int_d)
		{
			make<BigInteger, big_int_d>(std::move(other.get_big_int()));
		} else if (other.disc_==submap_d)
		{
			make<submap_t, submap_d>(std::move(other.get_submap()));
		} else if (other.disc_==sublist_d)
		{
			make<sublist_t, sublist_d>(std::move(other.get_sublist()));
		} else
			assert(false);

		return *this;
	}

	template<class Visitor> auto json_value::apply_visitor(
		Visitor &vis) -> decltype(vis())
	{
		if (disc_ == nil_d)
		{
			return vis();
		} else if (disc_==bool_d)
		{
			return vis(get_bool());
		} else if (disc_==int_d)
		{
			return vis(get_int());
		} else if (disc_==double_d)
		{
			return vis(get_double());
		} else if (disc_==string_d)
		{
			return vis(get_str());
		} else if (disc_==big_int_d)
		{
			return vis(get_big_int());
		} else if (disc_==submap_d)
		{
			return vis(get_submap());
		} else if (disc_==sublist_d)
		{
			return vis(get_sublist());
		} else
			assert(false);
	}

	template<class Visitor> auto json_value::apply_visitor(
		Visitor &vis) const -> decltype(vis())
	{
		if (disc_ == nil_d)
		{
			return vis();
		} else if (disc_==bool_d)
		{
			return vis(get_bool());
		} else if (disc_==int_d)
		{
			return vis(get_int());
		} else if (disc_==double_d)
		{
			return vis(get_double());
		} else if (disc_==string_d)
		{
			return vis(get_str());
		} else if (disc_==big_int_d)
		{
			return vis(get_big_int());
		} else if (disc_==submap_d)
		{
			return vis(get_submap());
		} else if (disc_==sublist_d)
		{
			return vis(get_sublist());
		} else
			assert(false);
	}

	inline json_value::json_value(json_value &&o)
	{
		disc_ = nil_d;

		if (o.disc_ == nil_d)
		{
			;
		} else if (o.disc_==bool_d)
		{
			make<bool, bool_d>(std::move(o.get_bool()));
		} else if (o.disc_==int_d)
		{
			make<int, int_d>(std::move(o.get_int()));
		} else if (o.disc_==double_d)
		{
			make<double, double_d>(std::move(o.get_double()));
		} else if (o.disc_==string_d)
		{
			make<jstring_t, string_d>(std::move(o.get_str()));
		} else if (o.disc_==big_int_d)
		{
			make<BigInteger, big_int_d>(std::move(o.get_big_int()));
		} else if (o.disc_==submap_d)
		{
			make<submap_t, submap_d>(std::move(o.get_submap()));
		} else if (o.disc_==sublist_d)
		{
			make<sublist_t, sublist_d>(std::move(o.get_sublist()));
		} else
			assert(false);
	}

	/*
	struct double_visitor
	{
		const json_value &r_;
		double_visitor(const json_value &r) : r_(r) {}

		bool operator()() //nil_t
		{
			return true;
		}

		template<class S> struct second_visitor
		{
			const S& left_val_;
			second_visitor(const S& left_val) : left_val_(left_val) {}

			bool operator()() //nil_t
			{
				return true;
			}

			bool operator()(const S &r)
			{
				return left_val_ < r;
			}

			template<class K> bool operator()(const K &r)
			{
				throw std::bad_cast();
			}
		};

		template<class T> bool operator()(const T &v)
		{
			second_visitor<T> vis(v);
			return r_.apply_visitor(vis);
		}
	};
	*/
}; //namespace sofadb

#endif //NATIVE_JSON_H
