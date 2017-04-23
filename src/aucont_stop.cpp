#include "utils.hpp"
#include "utils_parser.hpp"
#include "container_stop.hpp"
#include "log.hpp"

#include <boost/program_options.hpp>
#include <signal.h>

using std::string;
using std::exception;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	log_tag("aucont_stop");
	container_id id;
	int signal;

	try {
		po::options_description options("Options");
		options.add_options()
				("log-stderr", "add stderr as log target")
				("log-file", po::value<string>(), "add file as log target")
				("pid", po::value<pid_t>(&id.pid), "pid of a container")
				("signal", po::value<int>(&signal)->default_value(SIGTERM), "signal to stop a container")
		;

		po::positional_options_description positional;
		positional.add("pid", 1);
		positional.add("signal", 1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv)
				.options(options)
				.positional(positional)
				.extra_style_parser(&all_positional_style_parser)
				.run(),
			vm);
		po::notify(vm);

		if (vm.count("pid") != 1) {
			fprintf(stderr, "Missing pid!\n");
			return 1;
		}

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
	CALL_(ret, container_kill_and_wait(id, signal), "=(", return -1);

	return 0;
}
