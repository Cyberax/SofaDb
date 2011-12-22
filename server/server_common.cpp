#define BOOST_NO_IOSTREAM
#include "server_common.h"
#include <stdint.h>

using namespace sofadb;

std::string sofadb::read_str(socket_ptr_t sock)
{
	uint32_t ln;
	boost::asio::read(*sock, boost::asio::buffer((void*)&ln, 4));
	ln = ntohl(ln);

	std::string res;
	res.resize(ln);
	boost::asio::read(*sock, boost::asio::buffer(&res[0], res.size()));
	return res;
}

void write_str(socket_ptr_t sock, const std::string &str)
{
	if (str.length()>UINT_MAX)
		throw std::out_of_range("String is too big");
	uint32_t ln2 = htonl(str.length());
	boost::asio::write(*sock, boost::asio::buffer(&ln2, 4));
	boost::asio::write(*sock, boost::asio::buffer(str, str.size()));
}
