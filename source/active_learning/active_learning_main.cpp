/**
 * @file active_learning_main.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief The main subroutine that is starting the active learning.
 * @version 0.1
 * @date 2023-03-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "active_learning_main.h"
#include "parameters.h"
#include "lstar.h"
#include "inputdata.h"
#include "inputdatalocator.h"
#include "abbadingoparser.h"
#include "csvparser.h"

#include <stdexcept>

using namespace std;
using namespace active_learning_namespace;

const bool INPUT_FILE_SUL = true; // TODO: have better switch for that later

const vector<int> active_learning_namespace::get_alphabet(){
  if(INPUT_FILE_SUL){
    bool read_csv = false;
    //ifstream input_stream = get_input_stream();

    ifstream input_stream(INPUT_FILE);  
    cout << "Input file: " << INPUT_FILE << endl;

    if(!input_stream) {
        cerr << "Input file not found, aborting" << endl;
        exit(-1);
    } else {
        cout << "Using input file: " << INPUT_FILE << endl;
    }

    if(INPUT_FILE.compare(INPUT_FILE.length() - 4, INPUT_FILE.length(), ".csv") == 0){
        read_csv = true;
    }

    inputdata id;
    inputdata_locator::provide(&id);
  
    if(read_csv) {
        auto input_parser = csv_parser(input_stream, csv::CSVFormat().trim({' '}));
        id.read(&input_parser);
    } else {
        auto input_parser = abbadingoparser(input_stream);
        id.read(&input_parser);
    }
    input_stream.close();

    const vector<int> res = id.get_alphabet();
    return res;
  }
}

void active_learning_namespace::run_active_learning(){
  if(ACTIVE_LEARNING_ALGORITHM == "l_star"){
    const auto alphabet = get_alphabet();
    auto l_star = lstar_algorithm(alphabet);
    l_star.run_l_star();
  }
  else{
    throw logic_error("Fatal error: Unknown active_learning_algorithm flag used: " + ACTIVE_LEARNING_ALGORITHM);
  }
}