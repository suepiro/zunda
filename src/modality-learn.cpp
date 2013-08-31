#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "sentence.hpp"
#include "modality.hpp"
#include "eval.hpp"


bool mkdir(boost::filesystem::path dir_path) {
	boost::system::error_code error;
	const bool result = boost::filesystem::exists(dir_path, error);
	if (!result) {
		std::cerr << "mkdir " << dir_path.string() << std::endl;
		const bool res = boost::filesystem::create_directories(dir_path, error);
		if (!res || error) {
			std::cerr << "ERROR: mkdir " << dir_path.string() << " failed" << std::endl;
			return false;
		}
	}
	else if (error) {
		std::cerr << "ERROR" << std::endl;
		return false;
	}
	return true;
}


int main(int argc, char *argv[]) {

	unsigned int split_num = 5;

	boost::program_options::options_description opt("Usage");
	opt.add_options()
		("data,d", boost::program_options::value< std::vector<std::string> >()->multitoken(), "directory path containing learning data (required)")
		("input,i", boost::program_options::value<int>(), "format of learning data (optional)\n 0 - cabocha (default)\n 1 - XML")
		("ext,e", boost::program_options::value<std::string>(), "extension of learning files (optional)\n 0 - .deppasmod (default)\n 1 - .xml")
		("cross,x", "enable cross validation (optional): default off")
		("split,g", boost::program_options::value<unsigned int>(), "number of groups for cross validation (optional): default 5")
		("outdir,o", boost::program_options::value<std::string>(), "directory to store output files (optional)\n simple training -  stores model file and feature file to \"model (default)\"\n cross validation - stores model file, feature file and result file to \"output (default)\"")
		("target,t", boost::program_options::value<unsigned int>(), "target detection method\n 0 - by part of speech [default]\n 1 - predicate output by PAS (syncha format)\n 2 - by machine learning (has not been implemented)\n 3 - by gold data")
		("help,h", "Show help messages")
		("version,v", "Show version informaion");

	boost::program_options::variables_map argmap;
	boost::program_options::store(parse_command_line(argc, argv, opt), argmap);
	boost::program_options::notify(argmap);

	if (argmap.count("help")) {
		std::cout << opt << std::endl;
		return 1;
	}

	if (argmap.count("version")) {
		std::cout << MODALITY_VERSION << std::endl;
		return 1;
	}

	std::string outdir;
	if (argmap.count("outdir")) {
		outdir = argmap["outdir"].as<std::string>();
	}
	else {
		if (argmap.count("cross")) {
			outdir = "output";
		}
		else {
			outdir = "model";
		}
	}
	boost::filesystem::path outdir_path(outdir);
	mkdir(outdir_path);

	int input_layer = 0;
	if (argmap.count("input")) {
		input_layer = argmap["input"].as<int>();
	}
	std::string ext;
	switch (input_layer) {
		case 0:
			std::cout << "data format: cabocha" << std::endl;
			ext = ".deppasmod";
			break;
		case 1:
			std::cout << "data format: XML" << std::endl;
			ext = ".xml";
			break;
		default:
			std::cerr << "ERROR: invalid data format type" << std::endl;
			return false;
	}
	if (argmap.count("ext")) {
		ext = argmap["ext"].as<std::string>();
		if (ext.compare(0, 1, ".") != 0) {
			ext = "." + ext;
		}
	}

	if (!argmap.count("data")) {
		std::cerr << "ERROR: no data directory" << std::endl;
		return false;
	}

	std::vector<std::string> files;
	BOOST_FOREACH (std::string learn_data_dir, argmap["data"].as< std::vector<std::string> >() ) {
		const boost::filesystem::path learn_data_path(learn_data_dir);
		if (boost::filesystem::exists(learn_data_path)) {
			BOOST_FOREACH ( const boost::filesystem::path& p, std::make_pair(boost::filesystem::recursive_directory_iterator(learn_data_dir), boost::filesystem::recursive_directory_iterator()) ) {
				if (p.extension() == ext) {
					files.push_back(p.string());
				}
			}
		}
		else {
			std::cerr << "ERROR: no such directory: " << learn_data_dir << std::endl;
			return false;
		}
	}

	sort(files.begin(), files.end());
	files.erase(unique(files.begin(), files.end()), files.end());
	std::cout << "found " << files.size() << " files" << std::endl;

	if (files.size() == 0) {
		std::cerr << "ERROR: no learn data" << std::endl;
		exit(-1);
	}

	modality::parser mod_parser;
	mod_parser.closeDB();
	mod_parser.set_model_dir(outdir_path);
	mod_parser.openDB_writable();

	if (argmap.count("target")) {
		mod_parser.target_detection = argmap["target"].as<unsigned int>();
	}

	switch (input_layer) {
		case 0:
			mod_parser.load_deppasmods(files);
			break;
		case 1:
			mod_parser.load_xmls(files);
			break;
		default:
			std::cerr << "ERROR: invalid data format type" << std::endl;
			exit(-1);
	}
	std::cout << "load done: " << mod_parser.learning_data.size() << " sents" << std::endl;


	if (argmap.count("cross")) {
		evaluator evals[LABEL_NUM];
	
		if (argmap.count("x")) {
			split_num = argmap["x"].as<unsigned int>();
		}
		unsigned int grp_size = mod_parser.learning_data.size() / split_num;
		std::vector< std::vector< nlp::sentence > > split_data;

		for (unsigned int i=0 ; i<split_num ; ++i) {
			std::vector< nlp::sentence > grp;
			if (i+1 == split_num) {
				std::copy(mod_parser.learning_data.begin() + i*grp_size, mod_parser.learning_data.end(), std::back_inserter(grp));
			}
			else {
				std::copy(mod_parser.learning_data.begin() + i*grp_size, mod_parser.learning_data.begin() + (i+1)*grp_size, std::back_inserter(grp));
			}
			split_data.push_back(grp);
		}

		unsigned int cnt = 0;
		BOOST_FOREACH (std::vector<nlp::sentence> grp, split_data) {
			std::cout << " group " << cnt << ": " << grp.size() << std::endl;
			cnt++;
		}
		std::cout << std::endl;
		
		for (unsigned int step=0 ; step<split_num ; ++step) {
			std::cout << "* step " << step << std::endl;
			mod_parser.learning_data.clear();

			boost::filesystem::path model_path[LABEL_NUM];
			boost::filesystem::path feat_path[LABEL_NUM];
			boost::filesystem::path result_path[LABEL_NUM];

			BOOST_FOREACH (unsigned int i, mod_parser.analyze_tags) {
				std::stringstream suffix_ss;
				suffix_ss << std::setw(3) << std::setfill('0') << step;
				boost::filesystem::path mp("model_" + mod_parser.id2tag(i) + suffix_ss.str());
				model_path[i] = outdir_path / mp;
				boost::filesystem::path fp("feat_" + mod_parser.id2tag(i) + suffix_ss.str());
				feat_path[i] = outdir_path / fp;
				boost::filesystem::path rp("result_" + mod_parser.id2tag(i) + suffix_ss.str());
				result_path[i] = outdir_path / rp;
			}
			
			for (unsigned int i=0 ; i<split_num ; ++i) {
				if (i != step) {
					mod_parser.learning_data.insert(mod_parser.learning_data.end(), split_data[i].begin(), split_data[i].end());
				}
			}
			std::cout << " learning data size: " << mod_parser.learning_data.size() << std::endl;

			mod_parser.learn(model_path, feat_path);

			mod_parser.load_models(model_path);

			std::ofstream *os = new std::ofstream[LABEL_NUM];
			BOOST_FOREACH (unsigned int i, mod_parser.analyze_tags) {
				os[i].open(result_path[i].c_str());
			}

			std::vector< nlp::sentence > test_data = split_data[step];
			for (unsigned int set=0 ; set<test_data.size() ; ++set) {

				std::vector<int> tagged_tok_ids;

				// when target detection is DETECT_BY_GOLD, only bool value of having modality is set to test data
				if (mod_parser.target_detection == modality::DETECT_BY_GOLD) {
					BOOST_FOREACH (nlp::chunk chk, test_data[set].chunks) {
						BOOST_FOREACH (nlp::token tok, chk.tokens) {
							if (tok.has_mod) {
								tok.mod.tag.clear();
//								tagged_tok_ids.push_back(tok.id);
							}
						}
					}
				}
				else {
					test_data[set].clear_mod();
				}

				// when target detection is DETECT_BY_GOLD, only bool value of having modality is set to test data
				/*
				if (mod_parser.target_detection == modality::DETECT_BY_GOLD) {
					for (std::vector< nlp::chunk >::iterator it_chk=test_data[set].chunks.begin() ; it_chk!=test_data[set].chunks.end() ; ++it_chk) {
						for (std::vector< nlp::token >::iterator it_tok=it_chk->tokens.begin() ; it_tok!=it_chk->tokens.end() ; ++it_tok) {
							if ( std::find(tagged_tok_ids.begin(), tagged_tok_ids.end(), it_tok->id) != tagged_tok_ids.end() ) {
								it_tok->has_mod = true;
								it_chk->has_mod = true;
							}
						}
					}
				}
				*/

				// tokens to be analyzed are detected by specified method in analyze() and gold data validation
				nlp::sentence tagged_sent = mod_parser.analyze(test_data[set], false);

				for (unsigned int chk_cnt=0 ; chk_cnt<tagged_sent.chunks.size() ; ++chk_cnt) {
					for (unsigned int tok_cnt=0 ; tok_cnt<tagged_sent.chunks[chk_cnt].tokens.size() ; ++tok_cnt) {
						nlp::token tok_gold = split_data[step][set].chunks[chk_cnt].tokens[tok_cnt];
						nlp::token tok_sys = tagged_sent.chunks[chk_cnt].tokens[tok_cnt];
						if (tok_gold.has_mod && tok_sys.has_mod) {
							std::stringstream id_ss;
							id_ss << tagged_sent.sent_id << "_" << tok_sys.id;

							BOOST_FOREACH (unsigned int i, mod_parser.analyze_tags) {
								evals[i].add( id_ss.str() , tok_gold.mod.tag[mod_parser.id2tag(i)], tok_sys.mod.tag[mod_parser.id2tag(i)] );
								os[i] << id_ss.str() << "," << tok_gold.mod.tag[mod_parser.id2tag(i)] << "," << tok_sys.mod.tag[mod_parser.id2tag(i)] << std::endl;
							}
						}
						else if (tok_sys.has_mod && !tok_gold.has_mod) {
							std::cerr << "ERROR: modality tag does not exist in token " << tok_sys.id << " in " << test_data[set].sent_id << std::endl;
						}
					}
				}
			}

			BOOST_FOREACH (unsigned int i, mod_parser.analyze_tags) {
				os[i].close();
			}
		}

		BOOST_FOREACH (unsigned int i, mod_parser.analyze_tags) {
			std::cout << "* " << mod_parser.id2tag(i) << std::endl;
			evals[i].eval();
			double acc = evals[i].accuracy();
			std::cout << acc << std::endl;
			evals[i].print_confusion_matrix();
			evals[i].print_prec_rec();
		}

	}
	else {
		mod_parser.learn();
	}

	mod_parser.save_hashDB();

	return 1;
}

