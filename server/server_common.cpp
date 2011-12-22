#define BOOST_NO_IOSTREAM
#include "server_common.h"
#include <stdint.h>

using namespace sofadb;

std::string sofadb::read_str(socket_ptr_t sock)
{
	std::string res;
	res.resize(read_uint32(sock));
	boost::asio::read(*sock, boost::asio::buffer(&res[0], res.size()));
	return res;
}

void sofadb::write_str(socket_ptr_t sock, const std::string &str)
{
	if (str.length()>UINT_MAX)
		throw std::out_of_range("String is too big");
	write_uint32(sock, str.length());
	boost::asio::write(*sock, boost::asio::buffer(str, str.size()));
}

uint32_t sofadb::read_uint32(socket_ptr_t sock)
{
	uint32_t ln;
	boost::asio::read(*sock, boost::asio::buffer((void*)&ln, 4));
	return ntohl(ln);
}

void sofadb::write_uint32(socket_ptr_t sock, uint32_t val)
{
	uint32_t ln2 = htonl(val);
	boost::asio::write(*sock, boost::asio::buffer(&ln2, 4));
}
