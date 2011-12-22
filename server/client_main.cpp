#include "common.h"
#include <gflags/gflags.h>
#include <scope_guard.h>
#include <boost/asio.hpp>

#include <iostream>

using boost::asio::local::stream_protocol;
using namespace sofadb;
typedef boost::shared_ptr<stream_protocol::socket> socket_ptr_t;

DEFINE_string(socket_dir, "/tmp", "Listen socket path");
DEFINE_string(socket_name, "", "Listen socket name");

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

	boost::asio::io_service io_service;
	stream_protocol::socket socket(io_service);
	socket.connect(stream_protocol::endpoint(socket_file));
	VLOG_MACRO(1) << "Connecting socket to " << socket_file;

//	while(true)
//	{
//	}

	return 0;
}
