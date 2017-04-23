#include "utils.hpp"
#include "utils_parser.hpp"
#include "container.hpp"
#include "log.hpp"

#include <string>
#include <vector>
#include <boost/program_options.hpp>

using std::string;
using std::vector;
using std::exception;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	log_tag("aucont_start");
	container_opts opts;

	try {
		po::options_description options("Options");
		options.add_options()
				("daemonize,d", "daemonize")
				("net", po::value<string>(&opts.net_ip)->default_value(""), "container ip address")
				("cpu", po::value<int>(&opts.cpu_limit)->default_value(-1), "container cpu limut")
				("log-stderr", "add stderr as log target")
				("log-file", po::value<string>(), "add file as log target")
				("root", po::value<string>(), "root of container filesystem")
				("cmd", po::value<vector<string>>(), "command to run as container")
		;

		po::positional_options_description positional;
		positional.add("root", 1);
		positional.add("cmd", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv)
				.options(options)
				.positional(positional)
				.extra_style_parser(&all_positional_style_parser)
				.run(),
			vm);
		po::notify(vm);

		if (vm.count("root") != 1 || vm.count("cmd") == 0) {
			fprintf(stderr, "Missing root and/or cmd to run!\n");
			return 1;
		}

		opts.daemonize = vm.count("daemonize") != 0;
		opts.rootfs_path = vm["root"].as<string>();
		opts.init_cmd = vm["cmd"].as<vector<string>>();

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
	CALL_(ret, container(opts), "=(", return 1);

	return 0;
}
