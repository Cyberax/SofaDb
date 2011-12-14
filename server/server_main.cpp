#include "common.h"
#include <gflags/gflags.h>
#include <scope_guard.h>

DEFINE_string(socket_path, "~/.local/share/sofadb/=socket",
			  "Listen socket path");
DEFINE_int32(socket_backlog, 10, "Maximum socket backlog");

int main(int argc, char **argv)
{
	ON_BLOCK_EXIT(&google::ShutDownCommandLineFlags);
	google::SetUsageMessage("Usage");
	google::SetVersionString("SofaDB Server 0.1");
	google::ParseCommandLineFlags(&argc, &argv, true);

	printf("Parsed\n");
	return 0;
}
