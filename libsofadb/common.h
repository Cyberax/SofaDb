#ifndef COMMON_H
#define COMMON_H

#if defined _WIN32 || defined __CYGWIN__
  #ifdef LIBSOFADB_EXPORTS
	#ifdef __GNUC__
	  #define SOFADB_PUBLIC __attribute__ ((dllexport))
	#else
	  #define SOFADB_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
	#endif
  #else
	#ifdef __GNUC__
	  #define SOFADB_PUBLIC __attribute__ ((dllimport))
	#else
	  #define SOFADB_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
	#endif
  #endif
  #define SOFADB_LOCAL
#else
  #if __GNUC__ >= 4
	#define SOFADB_PUBLIC __attribute__ ((visibility ("default")))
	#define SOFADB_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
	#define SOFADB_PUBLIC
	#define SOFADB_LOCAL
  #endif
#endif

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 10
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <stdexcept>

#include <mutex>
typedef std::lock_guard<std::recursive_mutex> guard_t;

#include <glog/logging.h>
#define VLOG_MACRO(lev) if(VLOG_IS_ON(lev)) VLOG(lev)

#endif // COMMON_H
