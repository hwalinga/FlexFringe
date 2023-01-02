/// @file
/// \brief Command line parameter processing and entry point.

#include <cstdlib>
#include "greedy.h"
#include "state_merger.h"
#include "evaluate.h"
#include "dfasat.h"
#include <iostream>
#include "evaluation_factory.h"
#include <string>
#include "stream.h"
#include "interactive.h"
#include "searcher.h"
#include "predict.h"
#include "differencing.h"
#include "ensemble.h"

#include "loguru.hpp"
#include "CLI11.hpp"

#include "parameters.h"
#include "input/csvreader.h"
#include "input/abbadingoreader.h"

// this file is generated by collector.sh
// during make, so contributors don't have to
// touch anything than their own files
string COMMAND;
bool debugging_enabled = false;

/*
 * Input parameters, see 'man popt'
 */

void print_current_automaton(state_merger* merger, const string& output_file, const string& append_string){
    if (OUTPUT_TYPE == "dot" || OUTPUT_TYPE == "both") {
        merger->print_dot(output_file + append_string + ".dot");
    }
    if (OUTPUT_TYPE == "json" || OUTPUT_TYPE == "both") {
        merger->print_json(output_file + append_string + ".json");
    }
    if(OUTPUT_SINKS && !PRINT_WHITE){
        bool red_undo = PRINT_RED;
        PRINT_RED = false;
        bool white_undo = PRINT_WHITE;
        PRINT_WHITE= true;
        bool blue_undo = PRINT_BLUE;
        PRINT_BLUE = true;
        if (OUTPUT_TYPE == "dot" || OUTPUT_TYPE == "both") {
            merger->print_dot(output_file + append_string + "sinks.dot");
        }
        if (OUTPUT_TYPE == "json" || OUTPUT_TYPE == "both") {
            merger->print_json(output_file + append_string + "sinks.json");
        }
        PRINT_RED = red_undo;
        PRINT_WHITE = white_undo;
        PRINT_BLUE = blue_undo;
    }
}

evaluation_function* get_evaluation(){
    evaluation_function *eval = nullptr;
    if(debugging_enabled){
        for(auto & myit : *DerivedRegister<evaluation_function>::getMap()) {
            cout << myit.first << " " << myit.second << endl;
        }
    }
    try {
        eval = (DerivedRegister<evaluation_function>::getMap())->at(HEURISTIC_NAME)();
        std::cout << "Using heuristic " << HEURISTIC_NAME << std::endl;
        LOG_S(INFO) <<  "Using heuristic " << HEURISTIC_NAME;
    } catch(const std::out_of_range& oor ) {
        LOG_S(WARNING) << "No named heuristic found, defaulting back on -h flag";
        std::cerr << "No named heuristic found, defaulting back on -h flag" << std::endl;
    }
    return eval;
}

/**
 * @brief Main run method. Branches out based on the type of session to run.
 * 
 * Possible sessions: 
 * - batch
 * - stream 
 * - inter
 * 
 * @param param The parameters. 
 */
void run() {
    evaluation_function *eval = get_evaluation();

    ifstream input_stream(INPUT_FILE);
    if(!input_stream) {
        LOG_S(ERROR) << "Input file not found, aborting";
        std::cerr << "Input file not found, aborting" << std::endl;
        exit(-1);
    }

    bool read_csv = false;
    if(INPUT_FILE.compare(INPUT_FILE.length() - 4, INPUT_FILE.length(), ".csv") == 0){
        read_csv = true;
    }

    if(OUTPUT_FILE.empty()) OUTPUT_FILE = INPUT_FILE + ".ff";

    inputdata* id;
    if(read_csv) {
        id = new csv_inputdata();
    } else {
        id = new abbadingo_inputdata();
    }
    inputdata_locator::provide(id);
    id->read(input_stream);

    apta* the_apta = new apta();
    auto* merger = new state_merger(id, eval, the_apta);
    the_apta->set_context(merger);
    eval->set_context(merger);

    cout << "Creating apta " <<  "using evaluation class " << HEURISTIC_NAME << endl;

    if(OPERATION_MODE == "batch" || OPERATION_MODE == "greedy") {
        cout << "batch mode selected" << endl;
//        if(read_csv) id->read_csv_file(input_stream);
//        else id->read_abbadingo_file(input_stream);
        eval->initialize_before_adding_traces();
        id->add_traces_to_apta(the_apta);
        eval->initialize_after_adding_traces(merger);
        print_current_automaton(merger, OUTPUT_FILE, ".init");
        LOG_S(INFO) << "Greedy mode selected, starting run";
        // run the state merger
        greedy_run(merger);

        print_current_automaton(merger, OUTPUT_FILE, ".final");
    } else if(OPERATION_MODE == "satsolver") {
        cout << "satsolver mode selected" << endl;
//        if(read_csv) id->read_csv_file(input_stream);
//        else id->read_abbadingo_file(input_stream);
        eval->initialize_before_adding_traces();
        id->add_traces_to_apta(the_apta);
        eval->initialize_after_adding_traces(merger);
        LOG_S(INFO) << "Satsolver mode selected, starting run";

        run_dfasat(merger, SAT_SOLVER, -1);

        print_current_automaton(merger, OUTPUT_FILE, ".final");
    } else if(OPERATION_MODE == "stream") {
        cout << "stream mode selected" << endl;
        LOG_S(INFO) << "Stream mode selected, starting run";

        stream_object stream_obj;
        stream_obj.stream_mode(merger, input_stream, id);

        print_current_automaton(merger, OUTPUT_FILE, ".final");
    } else if(OPERATION_MODE == "search") {
        cout << "search mode selected" << endl;
//        if(read_csv) id->read_csv_file(input_stream);
//        else id->read_abbadingo_file(input_stream);
        eval->initialize_before_adding_traces();
        id->add_traces_to_apta(the_apta);
        eval->initialize_after_adding_traces(merger);
        LOG_S(INFO) << "Search mode selected, starting run";

        bestfirst(merger);

        print_current_automaton(merger, OUTPUT_FILE, ".final");
    } else if(OPERATION_MODE == "bagging") {
        cout << "bagging mode selected" << endl;
//        if(read_csv) id->read_csv_file(input_stream);
//        else id->read_abbadingo_file(input_stream);
        eval->initialize_before_adding_traces();
        id->add_traces_to_apta(the_apta);
        eval->initialize_after_adding_traces(merger);
        LOG_S(INFO) << "Bagging mode selected, starting run";

        bagging(merger, OUTPUT_FILE,10);
    } else if(OPERATION_MODE == "interactive") {
        cout << "interactive mode selected" << endl;
//        if(read_csv) id->read_csv_file(input_stream);
//        else id->read_abbadingo_file(input_stream);
        eval->initialize_before_adding_traces();
        id->add_traces_to_apta(the_apta);
        eval->initialize_after_adding_traces(merger);
        LOG_S(INFO) << "Interactive mode selected, starting run";

        interactive(merger);

        print_current_automaton(merger, OUTPUT_FILE, ".final");
    } else if(OPERATION_MODE == "predict") {
        cout << "predict mode selected" << endl;
        LOG_S(INFO) << "Predict mode selected, starting run";

        if(!APTA_FILE.empty()){
            ifstream input_apta_stream(APTA_FILE);
            cerr << "reading apta file - " << APTA_FILE << endl;
            the_apta->read_json(input_apta_stream);

            std::ostringstream res_stream;
            res_stream << APTA_FILE << ".result";
            ofstream output(res_stream.str().c_str());
            if(read_csv) predict_csv(merger, input_stream, output);
            else predict(merger, input_stream, output);
            output.close();
        } else {
            cerr << "require a json formatted apta file to make predictions" << endl;
        }
    } else if(OPERATION_MODE == "diff") {
        cout << "behavioral differencing mode selected" << endl;
        LOG_S(INFO) << "Diff mode selected, starting run";

        if(!APTA_FILE.empty() && !APTA_FILE.empty()){
            ifstream input_apta_stream(APTA_FILE);
            cerr << "reading apta file - " << APTA_FILE << endl;
            the_apta->read_json(input_apta_stream);

            apta* the_apta2 = new apta();
            ifstream input_apta_stream2(APTA_FILE2);
            cerr << "reading apta file - " << APTA_FILE2 << endl;
            the_apta2->read_json(input_apta_stream2);

            std::ostringstream res_stream;
            cout << symmetric_difference(the_apta, the_apta2) << endl;
            delete the_apta2;
        } else {
            cerr << "require two json formatted apta files to compare" << endl;
        }
    } else {
       LOG_S(ERROR) << "Unknown mode of operation selected, please chose batch, stream, or inter. Provided was " << OPERATION_MODE;
       cerr << R"(unknown mode of operation selected, valid options are "batch", "stream", and "inter", while the parameter provided was )" << OPERATION_MODE << endl;
       exit(1);
    }
    input_stream.close();
    delete merger;
}

/**
 * @brief Main method. Reads in arguments and starts application 
 * by running "run()" function with the set of parsed parameters.
 * 
 */
#ifndef UNIT_TESTING
int main(int argc, char *argv[]){

    for(int i = 0; i < argc; i++) {
      COMMAND_LINE += string(argv[i]) + string(" ");
    }

    //cout << "welcome, running git commit " << gitversion <<  " with: "<< param->command << endl;

    // CLI11-based parameter parsing to replace libpopt
    // https://cliutils.gitlab.io/CLI11Tutorial/
    CLI::App app{"flexFringe"
                 "Copyright 2022 Sicco Verwer, Delft University of Technology"
                 "with contributions from Christian Hammerschmidt, Delft University of Technology,\
                  University of Luxembourg"
                 "with contributions from APTA Technologies B.V."
                 "based on"
                 "DFASAT with random greedy preprocessing"
                 "Copyright 2015 Sicco Verwer and Marijn Heule, Delft University of Technology."
    };

    // remove -h short-form from help because for backward-compatibility --heuristic needs nit
    app.set_help_flag("--help", "Print this help message and exit");

    // read parameters from ini file if desired
    std::string default_file_name = "flexfringe.ini";
    app.add_option("tracefile", INPUT_FILE, "Name of the input file containing the traces, either in Abbadingo or JSON format.")->required();
    app.add_option("--outputfile", OUTPUT_FILE, "The prefix of the output file name. Default is same as input.");
    app.add_option("--output", OUTPUT_TYPE, "Switch between output in dot, json, or both (default) formats.");
    app.add_option("--logpath", LOG_PATH, "The path to write the flexfringe log file to. Defaults to \"flexfringe.log\"");
    app.set_config("--ini", default_file_name, "Read an ini file", false);
    app.add_option("--mode", OPERATION_MODE, "batch (default), interactive, or stream depending on the mode of operation.");
    app.add_option("--heuristic-name,--heuristic_name", HEURISTIC_NAME, "Name of the merge heuristic to use; default count_driven. Use any heuristic in the evaluation directory. It is often beneficial to write your own, as heuristics are very application specific.")->required();
    app.add_option("--data-name,--data_name", DATA_NAME, "Name of the merge data class to use; default count_data. Use any heuristic in the evaluation directory.");
    app.add_option("--evalpar", EVALUATION_PARAMETERS, "string of key-value pairs for evaluation functions");

    app.add_option("--satsolver", SAT_SOLVER, "Name of the SAT solver executable. Default=glucose.");
    app.add_option("--aptafile", APTA_FILE, "Name of the input file containing a previously learned automaton in json format. Note that you need to use the same evaluation function it was learned with.");
    app.add_option("--aptafile2", APTA_FILE2, "Name of the input file containing a previously learned automaton in json format. Note that you need to use the same evaluation function it was learned with.");

    app.add_option("--debug", DEBUGGING, "turn on debugging mode, printing includes pointers and find/union structure, more output");
    app.add_option("--addtails", ADD_TAILS, "Add tails to the states, used for splitting nodes. When not needed, it saves space and time to not add them. Default=1.");
    app.add_option("--random", RANDOMIZE_SCORES, "Amount of randomness r to include in merging heuristic. Each merge score s is modified to (s - s*random(0,r)). Default=0.");
    app.add_option("--runs", ENSEMBLE_RUNS, "Number of greedy runs/iterations; default=1. Advice: when using random greedy, a higher value is recommended (100 was used in Stamina winner).");
    app.add_option("--parentsizethreshold", PARENT_SIZE_THRESHOLD, "The minimum size for a node to be assigned children when reading inputs, subsequent events in longer traces are ignored. Useful for streaming and some evaluation functions. Default=-1.");
    app.add_option("--reversetraces", REVERSE_TRACES, "When set to true, flexfringe starts from a suffix instead of a prefix tree. Default = 0.");
    app.add_option("--slidingwindow", SLIDING_WINDOW, "Use a sliding window when reading CSV files. Default=0.");
    app.add_option("--swsize", SLIDING_WINDOW_SIZE, "The size of the sliding window if --slidingwindow is set to 1. Default=20.");
    app.add_option("--swstride", SLIDING_WINDOW_STRIDE, "The stride (jump size) between two sliding windows, when --slidingwindow is set to 1. Default=5.");
    app.add_option("--swtype", SLIDING_WINDOW_TYPE, "Whether the sliding window should use the last element as the sliding window type. Default = 0.");
    app.add_option("--swaddshorter", SLIDING_WINDOW_ADD_SHORTER, "Whether sliding windows shorter than swsize should be added to the apta. Default = 0.");
    app.add_option("--redbluethreshold", RED_BLUE_THRESHOLD, "Boolean. If set to 1, then states will only be appended to red- or blue states. Only makes sense in stream mode. Default=0.");

    app.add_option("--extend", EXTEND_ANY_RED, "When set to 1, any merge candidate (blue) that cannot be merged with any target (red) is immediately changed into a (red) target; default=1. If set to 0, a merge candidate is only changed into a target when no more merges are possible. Advice: unclear which strategy is best, when using statistical (or count-based) consistency checks, keep in mind that merge consistency between states may change due to other performed merges. This will especially influence low frequency states. When there are a lot of those, we therefore recommend setting x=0.");
    app.add_option("--shallowfirst", DEPTH_FIRST, "When set to 1, the ordering of the nodes is changed from most frequent first (default) to most shallow (smallest depth) first; default=0. Advice: use depth-first when learning from characteristic samples.");
    app.add_option("--largestblue", MERGE_MOST_VISITED, "When set to 1, the algorithm only tries to merge the most frequent (or most shallow if w=1) candidate (blue) states with any target (red) state, instead of all candidates; default=0. Advice: this reduces run-time significantly but comes with a potential decrease in merge quality");
    app.add_option("--blueblue", MERGE_BLUE_BLUE, "When set to 1, the algorithm tries to merge candidate (blue) states with candidate (blue) states in addition to candidate (blue) target (red) merges; default=0. Advice: this adds run-time to the merging process in exchange for potential improvement in merge quality.");
    app.add_option("--redfixed", RED_FIXED, "When set to 1, merges that add new transitions to red states are considered inconsistent. Merges with red states will also not modify any of the counts used in evaluation functions. Once a red state has been learned, it is considered final and unmodifiable; default=0. Advice: setting this to 1 frequently results in easier to visualize and more insightful models.");
    app.add_option("--allfixed", ALL_FIXED, "When set to 1, merges that add new transitions to any state are considered inconsistent. Merges with red states will also not modify any of the counts used in evaluation functions. Default=0. Advice: setting this to 1 leads to insightful but large models.");
    app.add_option("--ktail", KTAIL, "k-Tails (speedup parameter), only testing merges until depth k (although original ktails can produce non-deterministic machines, flexfringe cannot, it is purely for speedup). Default=-1");
    app.add_option("--origktail", IDENTICAL_KTAIL, "Original k-Tails, requires merged states to have identical suffixes (future paths) up to depth k. (although original ktails can produce non-deterministic machines, flexfringe cannot). Default=-1.");
    app.add_option("--kstate", KSTATE, "k-Tails for states (speedup parameter), only testing merges until states of size k. Default=-1.");
    app.add_option("--mergelocal", MERGE_LOCAL, "only perform local merges, up to APTA distance k, useful when learning from software data.");
    app.add_option("--mcollector", MERGE_LOCAL_COLLECTOR_COUNT, "when local merges are used, allow merges with non-local collector states, these are states with at least k input transitions.");
    app.add_option("--markovian", MARKOVIAN_MODEL, "learn a \"Markovian\" model that ensures the incoming transitions have the same label, resulting in a Markov graph (states correspond to a unique label, but the same label can occur in multiple places), any heuristic can be used. (default: 0)");
    app.add_option("--mergeroot", MERGE_ROOT, "Allow merges with the root? Default: 1 (true).");
    app.add_option("--testmerge", MERGE_WHEN_TESTING, "When set to 0, merge tries in order to compute the evaluation scores do not actually perform the merges themselves. Thus the consistency and score evaluation for states in merges that add recursive loops are uninfluenced by earlier merges; default=1. Advice: setting this to 0 reduces run-time and can be useful when learning models using statistical evaluation functions, but can lead to inconsistencies when learning from labeled data.");
    app.add_option("--mergedata", MERGE_DATA, "Whether to update data during the merging process (1) or keep the counts from the prefix tree intact (0). Default=1.");

    app.add_option("--sinkson", USE_SINKS, "Set to 1 to use sink states; default=1. Advice: leads to much more concise and easier to visualize models, but can cost predictive performance depending on the sink definitions.");
    app.add_option("--sinkcount", SINK_COUNT, "The maximum number of occurrences of a state for it to be a low count sink (see evaluation functions); default=10.");
    app.add_option("--mergesinks", MERGE_SINKS, "Whether to merge sinks with other sink nodes after the main merging process. default=0.");
    app.add_option("--mergesinkscore", MERGE_SINKS_WITH_CORE, "Whether to merge sinks with the red core (any other state) after the main merging process. default=0.");
    app.add_option("--satmergesinks", MERGE_SINKS_PRESOLVE, "Merge all sink nodes of the same type before sending the problem to the SAT solver (setting 0 or 1); default=1. Advice: radically improves runtime, only set to 0 when sinks of the same type can be different states in the final model.");
    app.add_option("--searchsinks", SEARCH_SINKS, "Start search process once all remaining blue states are sink nodes, use greedy before. Only valid for search strategies. Default 0 (false).");
    app.add_option("--sinkidentical", MERGE_IDENTICAL_SINKS, "Only merge sinks if they have identical suffixes. Default=0.");
    app.add_option("--convertsinks", CONVERT_SINK_STATES, "Instead of merging sinks, convert them to their form defined by the evaluation function (typically a garbage state). Default 0 (false).");
    app.add_option("--extendsinks", EXTEND_SINKS, "Only relevant when mergesinks is set to 1. When set to 1, sinks can be extended (aka, added to the core, colored red). When set to 0, all sinks will be merged with the current core. Default=1.");

    app.add_option("--finalprob", FINAL_PROBABILITIES, "model final probabilities? if set to 1, distributions are over Sigma*, otherwise over SigmaN. (default: 0)");
    app.add_option("--lowerbound", USE_LOWER_BOUND, "Does the merger use a minimum value of the heuristic function? Set using --lowerboundval. Default=0. Advice: state merging is forced to perform the merge with best heuristic value, it can sometimes be better to color a state red rather then performing a bad merge. This is achieved using a positive lower bound value. Models learned with positive lower bound are frequently more interpretable");
    app.add_option("--lowerboundval", LOWER_BOUND, "Minimum value of the heuristic function, smaller values are treated as inconsistent. Default=0. Advice: state merging is forced to perform the merge with best heuristic value, it can sometimes be better to color a state red rather then performing a bad merge. This is achieved using a positive lower bound value. Models learned with positive lower bound are frequently more interpretable");
    app.add_option("--extendscore", EXTEND_SCORE, "The score for an extend (not merge or split) refinement. Set this higher or equal to lowerboundval. Default=0.");
    app.add_option("--state_count", STATE_COUNT, "The minimum number of positive occurrences of a state for it to be included in overlap/statistical checks (see evaluation functions); default=25. Advice: low frequency states can have an undesired influence on statistical tests, set to at least 10. Note that different evaluation functions can use this parameter in different ways.");
    app.add_option("--symbol_count", SYMBOL_COUNT, "When set to 1, merge tries in order to compute the evaluation scores do not actually perform the merges themselves. Thus the consistency and score evaluation for states in merges that add recursive loops are uninfluenced by earlier merges; default=0. Advice: setting this to 1 reduces run-time and can be useful when learning models using statistical evaluation functions, but can lead to inconsistencies when learning from labeled data.");
    app.add_option("--correction", CORRECTION, "Value of a Laplace correction (smoothing) added to all symbol counts when computing statistical tests (in ALERGIA, LIKELIHOODRATIO, AIC, and KULLBACK-LEIBLER); default=0.0. Advice: unclear whether smoothing is needed for the different tests, more smoothing typically leads to smaller models.");
    app.add_option("--correction_seen", CORRECTION_SEEN, "Additional correction applied to seen values.");
    app.add_option("--correction_unseen", CORRECTION_UNSEEN, "Additional correction applied to unseen values.");
    app.add_option("--correction_per_seen", CORRECTION_PER_SEEN, "Additional correction, adds this correction to counts per seen value to both seen and unseen values.");
    app.add_option("--confidence_bound", CHECK_PARAMETER, "Extra parameter used during statistical tests, the significance level for the likelihood ratio test, the alpha value for ALERGIA; default=0.5. Advice: look up the statistical test performed, this parameter is not always the same as a p-value.");
    app.add_option("--typedist", TYPE_DISTRIBUTIONS, "Whether to perform tests on the type distributions of states. Default = 0.");
    app.add_option("--symboldist", SYMBOL_DISTRIBUTIONS, "Whether to perform tests on the symbol distributions of states. Default = 1.");
    app.add_option("--typeconsistent", TYPE_CONSISTENT, "Whether to enforce type consistency for states, i.e., to not merge positive states with negative ones. Default=1.");

    app.add_option("--satoffset", OFFSET, "DFASAT runs a SAT solver to find a solution of size at most the size of the partially learned DFA + E; default=5. Advice: larger values greatly increases run-time. Setting it to 0 is frequently sufficient (when the merge heuristic works well).");
    app.add_option("--satplus", EXTRA_STATES, "With every iteration, DFASAT tries to find solutions of size at most the best solution found + P, default=0. Advice: current setting only searches for better solutions. If a few extra states is OK, set it higher.");
    app.add_option("--satfinalred", TARGET_REJECTING, "Make all transitions from red states without any occurrences force to have 0 occurrences (similar to targeting a rejecting sink), (setting 0 or 1) before sending the problem to the SAT solver; default=0. Advice: the same as finalred but for the SAT solver. Setting it to 1 greatly improves solving speed.");
    app.add_option("--symmetry", SYMMETRY_BREAKING, "Add symmetry breaking predicates to the SAT encoding (setting 0 or 1), based on Ulyantsev et al. BFS symmetry breaking; default=1. Advice: in our experience this only improves solving speed.");
    app.add_option("--forcing", FORCING, "Add predicates to the SAT encoding that force transitions in the learned DFA to be used by input examples (setting 0 or 1); default=0. Advice: leads to non-complete models. When the data is sparse, this should be set to 1. It does make the instance larger and can have a negative effect on the solving time.");

    app.add_option("--printblue", PRINT_BLUE, "Print blue states in the .dot file? Default 1 (true).");
    app.add_option("--printwhite", PRINT_WHITE, "Print white states in the .dot file? These are typically sinks states, i.e., states that have not been considered for merging. Default 0 (false).");
    app.add_option("--outputsinks", OUTPUT_SINKS, "Print sink states and transition in a separate json file. Default 0 (false).");

    app.add_option("--depthcheck", PERFORM_DEPTH_CHECK, "In addition to standard state merging checks, perform a check layer-by-layer in the prefix tree. This is a try to get more information out of infrequent traces and aims to capture long-term dependencies. Default=0.");
    app.add_option("--symbolcheck", PERFORM_SYMBOL_CHECK, "In addition to standard state merging checks, perform a check symbol-by-symbol in the prefix tree. This is a try to get more information out of infrequent traces and aims to capture long-term dependencies. Default=0.");
    app.add_option("--depthcheckmaxdepth", DEPTH_CHECK_MAX_DEPTH, "In case of performing depth or symbol checks, this parameter gives the maximum depth to compute these tests for. Default=-1 (bounded by the prefix tree).");
    app.add_option("--mergecheck", PERFORM_MERGE_CHECK, "Perform the standard merge check from the core state-merging algorithm. When set to false, all merges evaluate to true except for other constraints such as locality, markovian, etc. Default=1.");

    app.add_option("--searchdeep", SEARCH_DEEP, "Search using a greedy call until no more merges can be performed. Default=0.");
    app.add_option("--searchlocal", SEARCH_LOCAL, "Search using the local heuristic from the evaluation function. Default=0.");
    app.add_option("--searchglobal", SEARCH_GLOBAL, "Search using the global heuristic from the evaluation function. Default=0.");
    app.add_option("--searchpartial", SEARCH_PARTIAL, "Search using the partial heuristic from the evaluation function. Default=0.");

    app.add_option("--predictreset", PREDICT_RESET, "When predicting and there is no outgoing transition, the model is reset to the root state. This works well when using sliding windows. Default=0.");
    app.add_option("--predictremain", PREDICT_REMAIN, "When predicting and there is no outgoing transition, the model is looped back into the current state. Default=0.");
    app.add_option("--predictalign", PREDICT_ALIGN, "When predicting and there is no outgoing transition, the model remaining trace is aligned to the model by jumping to any other state and skipping symbols. Default=0.");
    app.add_option("--predictminimum", PREDICT_MINIMUM, "Predict returns (or finds when aligning) the smallest probability of a symbol from a sequence. Default=0.");
    app.add_option("--predicttype", PREDICT_TYPE, "Predicting calls the predict type functions from the evaluation function. Default=0.");
    app.add_option("--predicttypepath", PREDICT_TYPE_PATH, "Predictings are made based on paths in addition to final states (if implemented by evaluation function). Default=0.");
    app.add_option("--predictsymbol", PREDICT_SYMBOL, "Predicting calls the predict symbol functions from the evaluation function. Default=0.");
    app.add_option("--predicttrace", PREDICT_TRACE, "Predicting calls the predict trace functions from the evaluation function. Default=1.");
    app.add_option("--predictdata", PREDICT_TRACE, "Predicting calls the predict data functions from the evaluation function. Default=0.");

    app.add_option("--aligndistancepenalty", ALIGN_DISTANCE_PENALTY, "A penalty for jumping during alignment multiplied by the merged prefix tree distance. Default: 0.0.");
    app.add_option("--alignskippenalty", ALIGN_SKIP_PENALTY, "A penalty for skipping during alignment. Default: 0.0.");

    app.add_option("--diffsize", DIFF_SIZE, "Behavioral differencing works by sampling diffsize traces and using these to compute KL-Divergence. Default=1000.");
    app.add_option("--diffmaxlength", DIFF_MAX_LENGTH, "The maximum length of traces sampled for differencing. Default=50.");
    app.add_option("--diffmin", DIFF_MIN, "The minimum score for the behavioral difference of a sampled trace. Default=-100.");

    // parameters specifically for CMS heuristic
    app.add_option("--numoftables", NROWS_SKETCHES, "Number of rows of sketches upon initialization.");
    app.add_option("--vectordimension", NCOLUMNS_SKETCHES, "Number of columns of sketches upon initialization.");
    app.add_option("--distancemetric", DISTANCE_METRIC_SKETCHES, "The distance metric when comparing the sketches. 1 hoeffding-bound and cosine-similarity for score, 2 hoeffding bound in both, 3 like 1 but pooled. Default: 1");
    app.add_option("--randominitialization", RANDOM_INITIALIZATION_SKETCHES, "If 0 (zero), then initialize CMS deterministically. Elsewise, CMS becomes random. Default: 0.");
    app.add_option("--futuresteps", NSTEPS_SKETCHES, "Number of steps into future when storing future in sketches. Default: 2.");

    CLI11_PARSE(app, argc, argv)

    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
    loguru::init(argc, argv);
    loguru::add_file(LOG_PATH.c_str(), loguru::Append, loguru::Verbosity_MAX);

    LOG_S(INFO) << "Starting flexfringe run";

    run();

    LOG_S(INFO) << "Ending flexfringe run normally";

    mem_store::erase();

    return 0;
}
#endif
