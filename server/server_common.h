#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include "common.h"
#include <boost/asio.hpp>

namespace sofadb {
	using boost::asio::local::stream_protocol;
	typedef boost::shared_ptr<stream_protocol::socket> socket_ptr_t;

	std::string read_str(socket_ptr_t sock);
	void write_str(socket_ptr_t sock, const std::string &str);
}; //namespace sofadb

#endif
