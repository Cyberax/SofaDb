#include "common.h"
#include "engine.h"
#include "errors.h"
#include "database.h"
#include "server_common.h"
#include <gflags/gflags.h>
#include <scope_guard.h>

#undef BOOST_HAS_RVALUE_REFS
#include <boost/thread.hpp>
#define BOOST_HAS_RVALUE_REFS

#include <iostream>

using boost::asio::local::stream_protocol;
using namespace sofadb;
//typedef boost::recursive_mutex::scoped_lock scoped_lock;
typedef boost::shared_ptr<stream_protocol::socket> socket_ptr_t;

DEFINE_string(socket_dir, "/tmp", "Listen socket path");
DEFINE_string(socket_name, "", "Listen socket name");
DEFINE_int32(socket_backlog, 10, "Maximum socket backlog");

struct engine_registry
{
	std::recursive_mutex lock_;
	std::map<std::string, engine_ptr> engines_;
};
typedef boost::shared_ptr<engine_registry> registry_ptr;

void session(registry_ptr registry, socket_ptr_t sock)
{
	try
	{
		//First, read the database file
		std::string dbfile=read_str(sock);

		engine_ptr engine;
		{
			guard_t g(registry->lock_);
			engine_ptr &ptr=registry->engines_[dbfile];
			if (!ptr)
				ptr.reset(new DbEngine(dbfile, false));

			engine=ptr;
		}

		database_ptr last_db;
		std::string last_db_name;
		for (;;)
		{
			std::string command = read_str(sock);
			std::string dbname = read_str(sock);
			if (dbname.empty())
				err(result_code_t::sError) << "Empty database";

			database_ptr db=(dbname==last_db_name)?
						last_db : engine->create_a_database(dbname);
			if (command=="GET")
			{

			} else if (command == "PUT")
			{
			} else
				err(result_code_t::sError) << "Unknown command " << command;
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

	registry_ptr registry(new engine_registry());
	while(true)
	{
		socket_ptr_t sock(new stream_protocol::socket(io_service));
		acceptor.accept(*sock);
		VLOG_MACRO(2) << "Accepted connection on " << socket_file;
		boost::thread t(boost::bind(&session, registry, _1) , sock);
	}

	return 0;
}
