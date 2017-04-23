#pragma once

#include <vector>
#include <boost/program_options.hpp>

std::vector<boost::program_options::option> all_positional_style_parser(
	std::vector<std::string>& args) {

	std::vector<boost::program_options::option> result;
	const std::string& tok = args[0];
	if (!tok.empty() && (tok[0] != '-')) {
		for (unsigned i = 0; i < args.size(); ++i) {
			boost::program_options::option opt;
			opt.value.push_back(args[i]);
			opt.original_tokens.push_back(args[i]);
			opt.position_key = INT_MAX;
			result.push_back(opt);
		}
		args.clear();
	}
	return result;
}
