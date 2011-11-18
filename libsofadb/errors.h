#ifndef ERRORS_H
#define ERRORS_H

#include "common.h"
#include <sstream>

namespace sofadb {
	class die_t {};
	extern SOFADB_PUBLIC const die_t die;

	class result_code_t
	{
	public:
		enum code_e {
			sOk = 200,
			sWrongRevision = 400,
			sNotFound = 404,
			sConflict = 409,
			sError = 500,
		};

		result_code_t() : code_(sOk), desc_()
		{
		}

		result_code_t(code_e code, const std::string &desc="None") :
			code_(code), desc_(desc)
		{

		}

		virtual ~result_code_t(){}

		code_e code() const { return code_; }
		const std::string& desc() const { return desc_; }
		bool ok() const {return code_==sOk;}
	private:
		code_e code_;
		std::string desc_;
	};
	extern SOFADB_PUBLIC const result_code_t sok;

	class sofa_exception : public virtual boost::exception,
			public std::exception
	{
		const result_code_t code_;
		std::string what_;
	public:
		SOFADB_PUBLIC sofa_exception(const result_code_t &code);
		virtual ~sofa_exception() throw() {}

		const result_code_t & err() const
		{
			return code_;
		}

		virtual const char* what() const throw()
		{
			return what_.c_str();
		}
	};

	/**
	  Usage - err(sOk) << "This is a description"
	  */
	struct err : public std::stringstream
	{
		err(result_code_t::code_e code) : code_(code) {}

		~err()
		{
			//Yes, we're throwing from a destructor, but that's OK since
			//if there's an exception already started then we have other
			//big problems.
			if (code_!=result_code_t::sOk)
			{
				boost::throw_exception(sofa_exception(
										   result_code_t(code_, str())));
			}
		}

	private:
		result_code_t::code_e code_;
	};

	inline void operator | (const result_code_t &code, const die_t &)
	{
		if (!code.ok())
			boost::throw_exception(sofa_exception(code));
	}

#define TRYIT(expr) try{ expr; } catch(const sofa_exception &ex) { \
		return ex.err(); \
	}

}; //namespace sofadb

#endif //ERRORS_H
