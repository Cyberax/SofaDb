#include "common.h"
#include <gflags/gflags.h>
#include <scope_guard.h>
#include <boost/asio.hpp>

#undef BOOST_HAS_RVALUE_REFS
#include <boost/thread.hpp>
#define BOOST_HAS_RVALUE_REFS

#include <iostream>

using boost::asio::local::stream_protocol;
using namespace sofadb;
typedef boost::shared_ptr<stream_protocol::socket> socket_ptr_t;

DEFINE_string(socket_dir, "/tmp", "Listen socket path");
DEFINE_string(socket_name, "", "Listen socket name");
DEFINE_int32(socket_backlog, 10, "Maximum socket backlog");

void session(socket_ptr_t sock)
{
	try
	{
		for (;;)
		{
//			char data[max_length];

//			boost::system::error_code error;
//			size_t length = sock->read_some(boost::asio::buffer(data), error);
//			if (error == boost::asio::error::eof)
//				break; // Connection closed cleanly by peer.
//			else if (error)
//				throw boost::system::system_error(error); // Some other error.

//			boost::asio::write(*sock, boost::asio::buffer(data, length));
		}
	} catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

int main(int argc, char **argv)
{
	ON_BLOCK_EXIT(&google::ShutDownCommandLineFlags);
	google::SetUsageMessage("Usage");
	google::SetVersionString("SofaDB Server 0.1");
	google::ParseCommandLineFlags(&argc, &argv, true);

	std::string socket_file = FLAGS_socket_dir;
	if (!socket_file.empty() && socket_file.at(socket_file.length()-1)!='/')
		socket_file.append(1, '/');
	if (FLAGS_socket_name.empty())
		socket_file.append("=").append(int_to_string(getuid()));
	else
		socket_file.append(FLAGS_socket_name);

	unlink(socket_file.c_str());

	boost::asio::io_service io_service;
	stream_protocol::acceptor acceptor(
				io_service, stream_protocol::endpoint(socket_file));
	VLOG_MACRO(1) << "Started acceptor on " << socket_file;

	acceptor.listen(FLAGS_socket_backlog);
	while(true)
	{
		socket_ptr_t sock(new stream_protocol::socket(io_service));
		acceptor.accept(*sock);
		boost::thread t(&session, sock);
	}

	return 0;
}
