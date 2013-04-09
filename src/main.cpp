#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
using boost::program_options::options_description;
using boost::program_options::value;
using boost::program_options::variables_map;
using boost::program_options::store;
using boost::program_options::parse_command_line;
using boost::program_options::notify;
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "sentence.hpp"
#include "modality.hpp"

int main(int argc, char *argv[]) {

	options_description opt("Usage");
	opt.add_options()
		("help,h", "Show help messages")
		("version,v", "Show version informaion");

	variables_map argmap;
	store(parse_command_line(argc, argv, opt), argmap);
	notify(argmap);

	if (argmap.count("help")) {
		std::cout << opt << std::endl;
		return 1;
	}

	if (argmap.count("version")) {
		std::cout << MODALITY_VERSION << std::endl;
		return 1;
	}

	std::string str;
	std::string buf;
	while( getline(std::cin, buf) ) {
		str += buf + "\n";
	}

	modality::parser mod_parser;
//	mod_parser.parse(str);

	boost::filesystem::directory_iterator pos("/home/junta-m/work/20110324/TeaM/XML/OC");
	boost::filesystem::directory_iterator last;
	for (; pos!=last ; ++pos) {
		boost::filesystem::path p(*pos);
		std::cout << p.string() << std::endl;
		mod_parser.learnOC(p.string());
	}
	return 1;
}

