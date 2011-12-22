#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include "common.h"
#include <boost/asio.hpp>

namespace sofadb {
	enum GetOptions
	{
		GET_BODY = 2,
		GET_REVINFO = 4,
		GET_REVLOG = 8,
	};

	using boost::asio::local::stream_protocol;
	typedef boost::shared_ptr<stream_protocol::socket> socket_ptr_t;

	std::string read_str(socket_ptr_t sock);
	void write_str(socket_ptr_t sock, const std::string &str);

	uint32_t read_uint32(socket_ptr_t sock);
	void write_uint32(socket_ptr_t sock, uint32_t val);
}; //namespace sofadb

#endif
