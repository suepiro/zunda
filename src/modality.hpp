#ifndef __MODALITY_HPP__
#define __MODALITY_HPP__

#include <iostream>
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>
//#include <mecab.h>
#include <cabocha.h>
#include <tinyxml2.h>
#include <linear.h>
#include <classias/classias.h>
#include <classias/classify/linear/multi.h>
#include <classias/data.h>
#include <classias/train/lbfgs.h>
#include <classias/train/online_scheduler.h>
#include <classias/feature_generator.h>
#include <classias/quark.h>
#include <kcpolydb.h>

#include "sentence.hpp"
#include "defaultmap.h"

#define MODALITY_VERSION 1.0

namespace modality {
	typedef boost::unordered_map< std::string, double > t_feat;

	typedef classias::train::lbfgs_logistic_multi<classias::msdata> trainer_type;
	typedef defaultmap<std::string, double> model_type;
	typedef classias::classify::linear_multi_logistic<model_type> classifier_type;

	enum {
		raw_text = 0,
		cabocha_text = 1,
		chapas_text = 2,
	};

class feature_generator
{
public:
    typedef std::string attribute_type;
    typedef std::string label_type;
    typedef std::string feature_type;

public:
    feature_generator()
    {
    }

    virtual ~feature_generator()
    {
    }

    inline bool forward(
        const std::string& a,
        const std::string& l,
        std::string& f
        ) const
    {
        f  = a;
        f += '\t';
        f += l;
        return true;
    }
};


/*	typedef classias::train::online_scheduler_multi<
		classias::msdata,
		classias::train::averaged_perceptron_multi<
			classias::classify::linear_multi<classias::weight_vector>
		>
	> trainer_type;
	*/

	typedef struct {
		int sp;
		int ep;
		std::string orthToken;
		std::string morphID;
		nlp::t_eme eme;
	} t_token;

	class instance {
		public:
			std::string label;
			t_feat *feat;

		public:
			instance() {
				feat = new t_feat;
			}
			~instance() {
			}
	};

		
	class parser {
		public:
			kyotocabinet::HashDB ttjDB;
			kyotocabinet::HashDB fadicDB;
			
//			MeCab::Tagger *mecab;
			CaboCha::Parser *cabocha;
			
			std::vector< nlp::sentence > learning_data;
			bool pred_detect_rule;
			
			parser() {
//				mecab = MeCab::createTagger("-p");
				if (!ttjDB.open("dic/ttjcore2seq.kch", kyotocabinet::HashDB::OREADER)) {
					std::cerr << "open error: ttjcore2seq: " << ttjDB.error().name() << std::endl;
				}
				if (!fadicDB.open("dic/FAdic.kch", kyotocabinet::HashDB::OREADER)) {
					std::cerr << "open error: fadic: " << fadicDB.error().name() << std::endl;
				}
				cabocha = CaboCha::createParser("-f1");
				
				pred_detect_rule = false;
			}
			~parser() {
			}

		public:
			void read_model(model_type&, classias::quark&, std::istream&);
			nlp::sentence classify(model_type, classias::quark, std::string, int);
//			bool parse(std::string);
			void load_xmls(std::vector< std::string >);
			void load_deppasmods(std::vector< std::string >);
			bool learnOC(std::string, std::string);

			nlp::sentence make_tagged_ipasents( std::vector< t_token > );

			std::vector< std::vector< t_token > > parse_OC(std::string);
			std::vector< std::vector< t_token > > parse_OW_PB_PN(std::string);
			std::vector< std::vector< t_token > > parse_OC_sents (tinyxml2::XMLElement *);
			std::vector<t_token> parse_bccwj_sent(tinyxml2::XMLElement *, int *);
			void parse_modtag_for_sent(tinyxml2::XMLElement *, std::vector< t_token > *);

			void gen_feature(nlp::sentence, int, t_feat &);
			void gen_feature_follow_mod(nlp::sentence, int, t_feat &);
			void gen_feature_function(nlp::sentence, int, t_feat &);
			void gen_feature_basic(nlp::sentence, int, t_feat &, int);
			void gen_feature_ttj(nlp::sentence, int, t_feat &);
			void gen_feature_fadic(nlp::sentence, int, t_feat &);
	};
};

#endif

