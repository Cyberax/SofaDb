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

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

#endif // COMMON_H
