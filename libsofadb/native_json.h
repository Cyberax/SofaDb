#ifndef NATIVE_JSON_H
#define NATIVE_JSON_H

#include "common.h"
#include <map>
#include <vector>
#include "BigInteger.hh"

#define MAX_ALIGNED_MEM 16

namespace sofadb {

	class json_value;
	typedef std::map<std::string, json_value> submap_t;
	typedef std::vector<json_value> sublist_t;

	namespace detail {
		union storage_t
		{
			char max_aligned[MAX_ALIGNED_MEM];
			char storage1[sizeof(std::string)];
			char storage3[sizeof(int64_t)];
			char storage2[sizeof(BigInteger)];
			char storage4[sizeof(double)];
			char storage5[sizeof(submap_t)];
			char storage6[sizeof(sublist_t)];
		};
	};

	enum json_disc {
		nil_d,
		bool_d,
		int_d,
		string_d,
		big_int_d,
		submap_d,
		sublist_d,
	};

	class json_value
	{
		json_disc disc_;
		detail::storage_t storage_;
	public:
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

		void as_nil() { clear(); }
		bool is_nil() { return disc_==nil_d; }

#define STD_FUNCS(type, type_postfix, disc) \
		json_value(const type &val) \
		{ \
			disc_ = nil_d; \
			make<type, disc>(val); \
		}\
		json_value(type &&val) \
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

		STD_FUNCS(bool, bool, bool_d)
		STD_FUNCS(int64_t, int, int_d)
		STD_FUNCS(std::string, str, string_d)
		STD_FUNCS(BigInteger, big_int, big_int_d)
		STD_FUNCS(submap_t, submap, submap_d)
		STD_FUNCS(sublist_t, sublist, sublist_d)

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
			new (storage_.max_aligned) T();
		}
		template<class T, json_disc D> void make(const T &ref)
		{
			assert(disc_ == nil_d);
			disc_ = D;
			new (storage_.max_aligned) T(ref);
		}
		template<class T, json_disc D> void make(T &&ref)
		{
			assert(disc_ == nil_d);
			disc_ = D;
			new (storage_.max_aligned) T(std::move(ref));
		}
	};

	SOFADB_PUBLIC std::ostream& operator << (std::ostream &str,
											 const json_value &val);

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	void json_value::clear()
	{
		if (disc_ == nil_d)
		{
			;
		} else if (disc_==bool_d)
		{
			;
		} else if (disc_==int_d)
		{
			;
		} else if (disc_==string_d)
		{
			using namespace std;
			get_str().~string();
		} else if (disc_==big_int_d)
		{
			get_big_int().~BigInteger();
		} else if (disc_==submap_d)
		{
			get_submap().~submap_t();
		} else if (disc_==sublist_d)
		{
			get_sublist().~sublist_t();
		} else
			assert(false);

		disc_ = nil_d;
	}

	inline json_value::json_value(const json_value& other)
	{
		clear();
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
		} else if (other.disc_==string_d)
		{
			make<std::string, string_d>(other.get_str());
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
		} else if (other.disc_==string_d)
		{
			make<std::string, string_d>(std::move(other.get_str()));
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

}; //namespace sofadb

#endif //NATIVE_JSON_H
