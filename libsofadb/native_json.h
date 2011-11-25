#ifndef NATIVE_JSON_H
#define NATIVE_JSON_H

#include "common.h"
#include <map>
#include <vector>

#define MAX_ALIGNED_MEM 16
#define SBO_LEN 32

namespace sofadb {

	class json_value;
	typedef std::map<jstring_t, json_value> submap_t;
	typedef std::vector<json_value> sublist_t;

	SOFADB_PUBLIC std::string int_to_string(int64_t in);

	struct bignum_t
	{
		jstring_t digits_;

		bignum_t() {}
		bignum_t(std::string &&digits) : digits_(digits) {}
		bignum_t(const std::string &digits) : digits_(digits) {}
		bignum_t(const bignum_t &other) : digits_(other.digits_) {}
		bignum_t(bignum_t &&other) : digits_(std::move(other.digits_)) {}
		bignum_t& operator = (const bignum_t& other)
		{
			digits_ = other.digits_;
			return *this;
		}
		bignum_t& operator = (bignum_t&& other)
		{
			if (this == &other) return *this;
			digits_ = std::move(other.digits_);
			return *this;
		}
	};

	SOFADB_PUBLIC bool operator < (const bignum_t &l, const bignum_t &r);
	inline bool operator > (const bignum_t &l, const bignum_t &r)
	{
		return r<l;
	}
	inline bool operator == (const bignum_t &l, const bignum_t &r)
	{
		return l.digits_ == r.digits_;
	}
	inline bool operator != (const bignum_t &l, const bignum_t &r)
	{
		return !(l == r);
	}

	struct graft_deleter_t
	{
		virtual void operator()(const json_value *val) = 0;
	};

	class graft_t
	{
		const json_value *graft_;
		graft_deleter_t *deleter_;
	public:
		SOFADB_PUBLIC graft_t();
		graft_t(const json_value *graft, graft_deleter_t *deleter=0) :
			graft_(graft), deleter_(deleter)
		{
			assert(graft);
		}
		graft_t(const graft_t &other) :
			graft_(other.graft_), deleter_(other.deleter_) {}
		graft_t& operator = (const graft_t& other)
		{
			graft_ = other.graft_;
			deleter_ = other.deleter_;
			return *this;
		}
		~graft_t()
		{
			if (graft_ && deleter_)
				(*deleter_)(graft_);
		}

		const json_value& grafted() const
		{
			assert(graft_);
			return *graft_;
		}
	};

	SOFADB_PUBLIC bool operator < (const graft_t &l, const graft_t &r);
	inline bool operator > (const graft_t &l, const graft_t &r)
	{
		return r<l;
	}
	SOFADB_PUBLIC bool operator == (const graft_t &l, const graft_t &r);
	inline bool operator != (const graft_t &l, const graft_t &r)
	{
		return !(l == r);
	}

	namespace detail {
		union storage_t
		{
			char max_aligned[MAX_ALIGNED_MEM];
			char storage1[sizeof(jstring_t)];
			char storage2[sizeof(int64_t)];
			char storage3[sizeof(double)];
			char storage4[sizeof(bignum_t)];
			char storage5[sizeof(submap_t)];
			char storage6[sizeof(sublist_t)];
			char storage7[sizeof(graft_t)];
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
		graft_d,
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
			bignum_t *_big_int_val;
			submap_t *_submap_val;
			sublist_t *_sublist_val;
			graft_t *_grafted_val;
		};
#endif
	public:
		SOFADB_PUBLIC static const json_value empty_val;

		json_value() { disc_ = nil_d; }
		json_value(json_disc type);
		json_value(const json_value& other);
		json_value(json_value &&);
		~json_value()
		{
			clear();
		}

		json_value& operator = (const json_value& other);
		json_value& operator = (json_value &&);

		json_disc type() const {return disc_;}
		SOFADB_PUBLIC json_value normalize_int() const;

		void as_nil() { clear(); }
		bool is_nil() { return disc_==nil_d; }

		const json_value& operator[](const jstring_t &str) const
		{
			return get_submap().at(str);
		}
		json_value& operator[](const jstring_t &str)
		{
			return get_submap()[str];
		}
		void insert(jstring_t &&key, json_value &&val)
		{
			get_submap().insert(std::make_pair(std::move(key), std::move(val)));
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
		STD_FUNCS(bignum_t, big_int, big_int_d,)
		STD_FUNCS(submap_t, submap, submap_d,)
		STD_FUNCS(sublist_t, sublist, sublist_d, )
		STD_FUNCS(graft_t, graft, graft_d, )

		template<class Visitor> auto apply_visitor(
			Visitor &vis) -> decltype(vis());
		template<class Visitor> auto apply_visitor(
			Visitor &vis) const -> decltype(vis());
	private:
		void assign(const json_value& other);
		void assign(json_value&& other);

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
				assert(disc_ == nil_d);
				make<T,D>();
			}
			return *(reinterpret_cast<T*>(storage_.max_aligned));
		}

		template<class T, json_disc D> void make()
		{
			disc_ = D;
#ifndef NDEBUG
			_str_val = reinterpret_cast<jstring_t*>(storage_.max_aligned);
#endif
			new (storage_.max_aligned) T();
		}
		template<class T, json_disc D> void make(const T &ref)
		{
			disc_ = D;
#ifndef NDEBUG
			_str_val = reinterpret_cast<jstring_t*>(storage_.max_aligned);
#endif
			new (storage_.max_aligned) T(ref);
		}
		template<class T, json_disc D> void make(T &&ref)
		{
			disc_ = D;
#ifndef NDEBUG
			_str_val = reinterpret_cast<jstring_t*>(storage_.max_aligned);
#endif
			new (storage_.max_aligned) T(std::move(ref));
		}
	};

	SOFADB_PUBLIC void json_to_string(jstring_t &append_to,
		const json_value &val, bool pretty = false);
	inline jstring_t json_to_string(const json_value &val, bool pretty = false)
	{
		jstring_t res;
		res.reserve(128);
		json_to_string(res, val, pretty);
		return res;
	}

	SOFADB_PUBLIC json_value string_to_json(const jstring_t &val);
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
				get_big_int().~bignum_t();
				break;
			case submap_d:
				get_submap().~submap_t();
				break;
			case sublist_d:
				get_sublist().~sublist_t();
				break;
			case graft_d:
				get_graft().~graft_t();
				break;
			default:
				assert(false);
		}
		disc_ = nil_d;
	}

	inline json_value::json_value(json_disc type)
	{
		switch(type)
		{
			case nil_d:
				disc_ = type;
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
				make<bignum_t, big_int_d>();
				break;
			case submap_d:
				make<submap_t, submap_d>();
				break;
			case sublist_d:
				make<sublist_t, sublist_d>();
				break;
			case graft_d:
				make<graft_t, graft_d>();
				break;
			default:
				assert(false);
		}
	}

	inline void json_value::assign(const json_value& other)
	{
		switch(other.disc_)
		{
		case nil_d:
			disc_ = other.disc_;
			break;
		case bool_d:
			make<bool, bool_d>(other.get_bool());
			break;
		case int_d:
			make<int64_t, int_d>(other.get_int());
			break;
		case double_d:
			make<double, double_d>(other.get_double());
			break;
		case string_d:
			make<jstring_t, string_d>(other.get_str());
			break;
		case big_int_d:
			make<bignum_t, big_int_d>(other.get_big_int());
			break;
		case submap_d:
			make<submap_t, submap_d>(other.get_submap());
			break;
		case sublist_d:
			make<sublist_t, sublist_d>(other.get_sublist());
			break;
		case graft_d:
			make<graft_t, graft_d>(other.get_graft());
			break;
		default:
			assert(false);
		}
		assert(disc_ == other.disc_);
	}

	inline json_value::json_value(const json_value& other)
	{
		assign(other);
//		if (other.type()!=nil_d && other.type()!=int_d)
//		{
//			backtrace_it();
//		}
	}

	inline json_value& json_value::operator = (const json_value& other)
	{
		if (&other == this)
			return *this;
		clear();
		assign(other);
		return *this;
	}

	inline void json_value::assign(json_value &&other)
	{
		switch(other.disc_)
		{
		case nil_d:
			disc_ = other.disc_;
			break;
		case bool_d:
			make<bool, bool_d>(other.get_bool());
			break;
		case int_d:
			make<int64_t, int_d>(other.get_int());
			break;
		case double_d:
			make<double, double_d>(other.get_double());
			break;
		case string_d:
			make<jstring_t, string_d>(std::move(other.get_str()));
			break;
		case big_int_d:
			make<bignum_t, big_int_d>(std::move(other.get_big_int()));
			break;
		case submap_d:
			make<submap_t, submap_d>(std::move(other.get_submap()));
			break;
		case sublist_d:
			make<sublist_t, sublist_d>(std::move(other.get_sublist()));
			break;
		case graft_d:
			make<graft_t, graft_d>(std::move(other.get_graft()));
			break;
		default:
			assert(false);
		}
		assert(disc_ == other.disc_);
	}

	inline json_value::json_value(json_value &&o)
	{
		assign(std::move(o));
	}

	inline json_value& json_value::operator = (json_value&& other)
	{
		if (&other == this)
			return *this;
		clear();
		assign(std::move(other));
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
		} else if (disc_==graft_d)
		{
			return vis(get_graft());
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
		} else if (disc_==graft_d)
		{
			return vis(get_graft());
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
