#include "utils.hpp"
#include "utils_parser.hpp"
#include "container_enter.hpp"
#include "log.hpp"

#include <signal.h>
#include <boost/program_options.hpp>

using std::string;
using std::vector;
using std::exception;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	log_tag("aucont_exec");
	container_id id;
	vector<string> cmd;

	try {
		po::options_description options("Options");
		options.add_options()
				("log-stderr", "add stderr as log target")
				("log-file", po::value<string>(), "add file as log target")
				("pid", po::value<pid_t>(&id.pid), "pid of a container")
				("cmd", po::value<vector<string>>(), "command to execute in a container")
		;

		po::positional_options_description positional;
		positional.add("pid", 1);
		positional.add("cmd", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv)
				.options(options)
				.positional(positional)
				.extra_style_parser(&all_positional_style_parser)
				.run(),
			vm);
		po::notify(vm);

		if (vm.count("pid") != 1 || vm.count("cmd") == 0) {
			fprintf(stderr, "Missing pid and/or cmd to run\n");
			return 1;
		}

		cmd = vm["cmd"].as<vector<string>>();

		if (vm.count("log-stderr"))
			log_insert(stderr);
		if (vm.count("log-file")) {
			string const& path = vm["log-file"].as<string>();
			FILE* f;
			if ((f = fopen(path.c_str(), "wt")) == NULL)
				return -1;
			log_insert(f);
		}
	} catch (exception& e) {
		log_insert(stderr);
		print_log("Fatal error: %s\n", e.what());
		return -1;
	}

	int ret;
	CALL_(ret, container_enter(id, cmd), "=(", return -1);

	return 0;
}

