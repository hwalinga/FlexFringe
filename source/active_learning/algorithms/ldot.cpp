/**
 * @file ldot.h
 * @author Hielke Walinga (hielkewalinga@gmail.com)
 * @brief The Ldot-algorithm, as part of my thesis
 * @version 0.1
 * @date 2024-1-9
 *
 * @copyright Copyright (c) 2024
 *
 * This code is heavily inspired (and copied) by the lsharp.cpp implementation by Robert Baumgartner.
 */

#include "ldot.h"
#include "common_functions.h"
#include "evaluate.h"
#include "input/trace.h"
#include "main_helpers.h"
#include "misc/printutil.h"
#include "parameters.h"
#include "refinement.h"
#include "sqldb_sul.h"
#include "sqldb_sul_random_oracle.h"
#include "utility/loguru.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <memory>
#include <vector>

/** Debugging function */
template <typename T> std::string printAddress(const std::list<T>& v) {
    stringstream ss;
    ss << "[";
    for (auto& e : v)
        ss << &e << ", ";
    ss << "]\n";
    return ss.str();
}

/** Debugging function */
void ldot_algorithm::test_access_traces() {
    return; // For debug
    for (APTA_iterator Ait = APTA_iterator(my_apta->get_root()); *Ait != 0; ++Ait) {
        apta_node* n = *Ait;
        apta_node* access_node = my_merger->get_state_from_trace(n->get_access_trace());
        /* if (access_node == nullptr) */
        /*     continue; */
        assert(access_node->find() == n->find());
    }
}

void ldot_algorithm::proc_counter_record(inputdata& id, const psql::record& rec, const refinement_list& refs) {
    active_learning_namespace::reset_apta(my_merger.get(), refs);
#ifndef NDEBUG
    DLOG_S(1) << n_runs << ": Undoing (for cex) " << refs.size() << " refs.";
    // printing the model each time
    print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.", std::to_string(n_runs) + ".zzz");
#endif

    performed_refinements.clear();

    vector<int> substring;
    apta_node* n = my_apta->get_root();

    if (COMPLETE_PATH_CEX) {
        for (const int s : rec.trace) {
            // Look for fringe, then add to apta.
            if (n != nullptr) {
                substring.push_back(s);
                n = active_learning_namespace::get_child_node(n, s);
                // apta_node n and substring point now to same place in apta.
            } else {
                // apta_node n points to non existent place in apta.
                auto record_maybe = my_sul->query_trace_opt(substring);
                if (record_maybe) {
                    add_trace(id, record_maybe.value());
                }
                substring.push_back(s);
            }
        }
    }

    // Now, add the counterex itself.
    add_trace(id, rec);

    // Now let's walk over the apta again, completing all the states we created.
    if (EXPLORE_OUTSIDE_CEX) {
        n = my_apta->get_root();
        for (auto s : rec.trace) {
            n = active_learning_namespace::get_child_node(n, s);
            complete_state(id, n);
        }
    }
}

trace* ldot_algorithm::add_trace(inputdata& id, const psql::record& r) {
    added_traces.insert(r.pk);

    LOG_S(1) << "pk:" << r.pk << " " << r.type << " " << r.trace;

    trace* new_trace = active_learning_namespace::vector_to_trace(r.trace, id, r.type);
    id.add_trace_to_apta(new_trace, my_merger->get_aut(), false);
    id.add_trace(new_trace);
    return new_trace;
}

void ldot_algorithm::merge_processed_ref(refinement* ref) {
#ifndef NDEBUG
    print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.",
                            std::to_string(n_runs) + fmt::format(".f{:0>4}", uid++) + ".pro_a");
    ref->doref(my_merger.get());
    performed_refinements.push_back(ref);
    print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.",
                            std::to_string(n_runs) + fmt::format(".f{:0>4}", uid++) + ".pro_b");

    DLOG_S(2) << n_runs << ":uni:" << printAddress(performed_refinements);
#else
    ref->doref(my_merger.get());
    performed_refinements.push_back(ref);
#endif
}

void ldot_algorithm::process_unidentified(inputdata& id, const std::vector<refinement_set>& refs_for_unidentified) {
    // SOME HEURISTIC THAT BALANCES EXPLORATION VS EXPLOITATION.
    // What to do when a state is unidentified (subroutine LdotProcessUnidentified). Here we take some
    // different approaches and we should see what works best. Additionally, what is in the unidentified
    // portion might prove that more exploration is needed, instead of building the hypothesis
    // (exploitation part).
    // Another idea:
    // Keep track of the explored depth of a state and if a state has a very low explored depth, we need
    // to increase the depth that is explored for that state.
    // For unidentified states, we might already have a very high merge probability. We can then make this
    // merge.

    for (const auto possible_refs : refs_for_unidentified) {
        refinement_set consistent_possible_refs;
        for (const auto ref : possible_refs) {
            if (ref->test_ref_consistency(my_merger.get())) {
                consistent_possible_refs.insert(ref);
            } else {
                ref->erase();
            }
        }
        if (consistent_possible_refs.empty()) {
            isolated_states = true;
            continue;
        }
        if (consistent_possible_refs.size() == 1) {
            refinement* best_merge = *consistent_possible_refs.begin();
            merge_processed_ref(best_merge);
            continue;
        }

        // compare more than 2.

        // Take two best and see what we can do
        auto it = consistent_possible_refs.begin();
        refinement* best_merge = *it;
        it++;
        refinement* sec_merge = *it;

        if (sec_merge->score * BEST_MERGE_THRESHOLD < best_merge->score) {
            // best merge has clearly much better score than sec merge.
            merge_processed_ref(best_merge);
        } else {
            // Not clear, gather more information
            if (DISTINGUISHING_MERGE_TEST) {
                throw std::runtime_error("Not implemented");
                // TODO:
                // Idea: Do this for all representatives.
                /* auto rec_maybe = */
                /*     my_sul->distinguishing_query(best_merge->red->access_trace, best_merge->blue->access_trace); */
                /* if (rec_maybe) { */
                /*     traces_to_add(rec_maybe.value()); */
                /* } else { */
                /*     merge_processed_ref(best_merge); */
                /* } */
            } else {
                merge_refinement* best_merge_ref = dynamic_cast<merge_refinement*>(best_merge);
                merge_refinement* sec_merge_ref = dynamic_cast<merge_refinement*>(sec_merge);

                std::vector<bool> complete_checks;

                // Check for the blue node and the red nodes.
                // The blue node is the same.
                if (COMPLETE_REPRESENTED) {
                    for (apta_node* n : represented_by(best_merge_ref->blue)) {
                        complete_checks.push_back(maybe_list_for_completion(n));
                    }
                    for (apta_node* n : represented_by(best_merge_ref->red)) {
                        complete_checks.push_back(maybe_list_for_completion(n));
                    }
                    for (apta_node* n : represented_by(sec_merge_ref->red)) {
                        complete_checks.push_back(maybe_list_for_completion(n));
                    }
                } else {
                    complete_checks = {maybe_list_for_completion(best_merge_ref->blue),
                                       maybe_list_for_completion(best_merge_ref->red),
                                       maybe_list_for_completion(sec_merge_ref->red)};
                }

                bool all_already_completed =
                    std::all_of(std::begin(complete_checks), std::end(complete_checks), [](bool i) { return i; });

                if (all_already_completed) {
                    // merge the best anyway, there cannot be more information found.
                    merge_processed_ref(best_merge);
                } else {
                    best_merge->erase();
                }
            }
        }
        while (it != consistent_possible_refs.end()) {
            refinement* ref = *it;
            ref->erase();
            it++;
        }
    }
}

// Returns true if already completed
bool ldot_algorithm::maybe_list_for_completion(apta_node* n) {
    if (!completed_nodes.contains(n)) {
        complete_these_states.insert(n);
        return false;
    }
    return true;
}

bool ldot_algorithm::complete_state(inputdata& id, apta_node* n) {
    if (completed_nodes.contains(n))
        return false;

    auto* access_trace = n->get_access_trace();
    auto seq = access_trace->get_input_sequence(true, true);
    for (const auto& rec : my_sul->prefix_query(seq, PREFIX_SIZE)) {
        add_trace(id, rec);
    }

    // Old code for random search.
    /* for (const int symbol : alphabet) { */
    /*     if (n->get_child(symbol) == nullptr) { */
    /*         auto* access_trace = n->get_access_trace(); */

    /*         active_learning_namespace::pref_suf_t seq = {symbol}; */
    /*         if (n->get_number() != -1) { */
    /*             seq = access_trace->get_input_sequence(true, true); */
    /*             seq[seq.size() - 1] = symbol; */
    /*         } */

    /*         const int answer = my_sul->query_trace_maybe(seq); */
    /*         if (answer != -1) */
    /*             add_trace(id, seq, answer); */
    /*     } */
    /* } */

    completed_nodes.insert(n);
    return true;
}

std::vector<apta_node*> ldot_algorithm::represented_by(apta_node* n) {
    n = n->find();
    std::vector<apta_node*> res;
    std::queue<apta_node*> nodes_to_process;
    nodes_to_process.push(n);
    while (!nodes_to_process.empty()) {

        n = nodes_to_process.front();
        nodes_to_process.pop();

        res.push_back(n);

        if (n->get_merged_head() != nullptr) {
            nodes_to_process.push(n->get_merged_head());
        }
        if (n->get_next_merged() != nullptr) {
            nodes_to_process.push(n->get_next_merged());
        }
    }
    return res;
}

void ldot_algorithm::run(inputdata& id) {
#ifndef NDEBUG
    LOG_S(INFO) << "Creating debug directory " << DEBUG_DIR;
    std::filesystem::create_directories(DEBUG_DIR);
#endif
    LOG_S(INFO) << "Running the ldot algorithm.";
    my_sul = dynamic_pointer_cast<sqldb_sul>(sul);
    if (my_sul == nullptr) {
        throw logic_error("ldot only works with sqldb_sul.");
    }

    unique_ptr<sqldb_sul_regex_oracle> my_oracle(dynamic_cast<sqldb_sul_regex_oracle*>(oracle.get()));
    if (my_oracle == nullptr) {
        throw logic_error("ldot only accepts the sqldb_sul_regex_oracle, there are more oracles, but a refactoring is "
                          "needed in that case.");
    }
    oracle.release(); // illegal ownership transfer of a unique_ptr (TODO: oracle should be shared_ptr)

    random_oracle = make_unique<sqldb_sul_random_oracle>(sul);

    n_runs = 1;
    n_subs = 1;

    // Initialize data structures.
    my_eval = unique_ptr<evaluation_function>(get_evaluation());
    my_eval->initialize_before_adding_traces();
    my_apta = make_unique<apta>();
    my_merger = make_unique<state_merger>(&id, my_eval.get(), my_apta.get());
    my_apta->set_context(my_merger.get());
    my_eval->set_context(my_merger.get());
    const vector<int> my_alphabet = id.get_alphabet();

    // init the root node, s.t. we have blue states to iterate over
    complete_state(id, my_apta->get_root());

#ifndef NDEBUG
    print_current_automaton(my_merger.get(), DEBUG_DIR + "/debug", "");
#endif

    std::vector<int> prev_cex = {-1};

    while (ENSEMBLE_RUNS > 0 && n_runs <= ENSEMBLE_RUNS) {
        LOG_S(INFO) << "Iteration " << n_runs << "." << n_subs;

        isolated_states = false; // Continue untill all isolated states are gone.
        // The refinements for the unidentified nodes.
        std::vector<refinement_set> refs_for_unidentified;

#ifndef NDEBUG
        // printing the model each time (once per n_runs).
        static int x = -1;
        if (x != n_runs)
            print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.", std::to_string(n_runs) + ".bef");
        x = n_runs;
#endif

        for (blue_state_iterator b_it = blue_state_iterator(my_apta->get_root()); *b_it != nullptr; ++b_it) {
            auto* blue_node = *b_it;

            /* if (blue_node->get_size() == 0) */
            /*     throw logic_error("This should never happen: TODO: Delete line"); */

            /* if (BLUE_NODE_COMPLETION) */
            /*     maybe_list_for_completion(blue_node); */

            refinement_set possible_refs;
            for (red_state_iterator r_it = red_state_iterator(my_apta->get_root()); *r_it != nullptr; ++r_it) {
                auto* const red_node = *r_it;

                refinement* ref = my_merger->test_merge(red_node, blue_node);
                if (ref != nullptr) {
                    // Collect all possible refinements.
                    possible_refs.insert(ref);
                }
            }

            if (possible_refs.empty()) {
                // Promotion of the blue node.
                isolated_states = true;
                refinement* ref = mem_store::create_extend_refinement(my_merger.get(), blue_node);

#ifndef NDEBUG
                test_access_traces();
                print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.",
                                        std::to_string(n_runs) + fmt::format(".f{:0>4}", uid++) + ".ext_a");
                ref->doref(my_merger.get());
                performed_refinements.push_back(ref);
                print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.",
                                        std::to_string(n_runs) + fmt::format(".f{:0>4}", uid++) + ".ext_b");
                DLOG_S(2) << n_runs << ":extend:" << printAddress(performed_refinements);
#else
                ref->doref(my_merger.get());
                performed_refinements.push_back(ref);
#endif
            }

            if (possible_refs.size() == 1) {
                // There is one refinement, do that one.
                refinement* ref = *possible_refs.begin();
#ifndef NDEBUG
                test_access_traces();
                print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.",
                                        std::to_string(n_runs) + fmt::format(".f{:0>4}", uid++) + ".one_a");
                ref->doref(my_merger.get());
                performed_refinements.push_back(ref);
                print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.",
                                        std::to_string(n_runs) + fmt::format(".f{:0>4}", uid++) + ".one_b");
                DLOG_S(2) << n_runs << ":one:" << printAddress(performed_refinements);
#else
                ref->doref(my_merger.get());
                performed_refinements.push_back(ref);
#endif
            }

            if (possible_refs.size() > 1) {
                // Check these possible refinements later to perform or to explore more.
                refs_for_unidentified.push_back(possible_refs);
            }
        }

        if (!refs_for_unidentified.empty()) {
            // Select based on some criteria some refs
            // Also might make some queries to get additional info for these unidentified blue nodes.
            // Maybe move this into the blue iterator loop?
            process_unidentified(id, refs_for_unidentified);
        }

        if (isolated_states) {
            // There are still isolated states that might need to be resolved.
            continue;
        }

        if (!complete_these_states.empty()) {
            // Complete states. NB First the whole APTA needs to be unrolled, then the information should be added.
            active_learning_namespace::reset_apta(my_merger.get(), performed_refinements);
#ifndef NDEBUG
            DLOG_S(1) << n_runs << ": Undoing (for completion) " << performed_refinements.size() << " refs.";
            // printing the model each time
            print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.", std::to_string(n_runs) + ".com");
#endif
            for (auto* ref : performed_refinements)
                ref->erase();

            performed_refinements.clear();

            for (auto* n : complete_these_states)
                complete_state(id, n);

            complete_these_states.clear();
            n_subs++;
            continue;
        }

        // MERGING FINISHED, CONTINUE WITH EQUIVALENCE ORACLE;

#ifndef NDEBUG
        test_access_traces();
        const std::size_t selected_refs_nr = performed_refinements.size();
        // printing the model each time
        print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.", std::to_string(n_runs) + ".sel");
#endif

        // build hypothesis -> try to reduce the apta.
        active_learning_namespace::minimize_apta(performed_refinements, my_merger.get());

#ifndef NDEBUG
        DLOG_S(1) << n_runs << ": Got " << selected_refs_nr << " refs from selection and a total of "
                  << performed_refinements.size() << " after minimizing.";
        // printing the model each time
        print_current_automaton(my_merger.get(), DEBUG_DIR + "/model.", std::to_string(n_runs) + ".xxx");
#endif

        // Check if we go for a next run.
        if (ENSEMBLE_RUNS > 0 && n_runs == ENSEMBLE_RUNS) {
            LOG_S(INFO) << n_runs << ":END: Maximum of runs reached. Printing automaton to " << OUTPUT_FILE;
            print_current_automaton(my_merger.get(), OUTPUT_FILE, ".final");
            return;
        }

        // Check equivalence, if not process the counter example.
        while (true) {
            /* While loop to check the type. type is < 0 if sul cannot properly respond to query, e.g. when the
            string we ask cannot be parsed in automaton. We ignore those cases, as they lead to extra states in
            hypothesis. This puts a burden on the equivalence oracle to make sure no query is asked twice, else we
            end up in infinite loop.*/

            try {

                optional<psql::record> query_result = equivalence(my_oracle.get());

                if (!query_result) {
                    LOG_S(INFO) << n_runs << ":END: Found consistent automaton => Print to " << OUTPUT_FILE;
                    print_current_automaton(my_merger.get(), OUTPUT_FILE,
                                            ".final"); // printing the final model.
                    return;                            // Solved!
                }
                auto cex_rec = query_result.value();
                const int type = cex_rec.type;
                // If this query could not be found ask for a new counter example.
                if (type < 0)
                    continue;

                const vector<int>& cex = cex_rec.trace;

                if (cex == prev_cex) {
                    throw std::runtime_error("repeated cex");
                }
                prev_cex = cex;

                const char* cex_log = ": Counterexample of length ";
                LOG_S(INFO) << n_runs << cex_log << cex.size() << " found: " << type << ": " << cex;
#ifndef NDEBUG
                std::cout << n_runs << cex_log << cex.size() << " found: " << type << ": " << cex << std::endl;
#endif
                proc_counter_record(id, cex_rec, performed_refinements);
                break;

            } catch (const std::runtime_error& e) {

                if (DISTINGUISHING_EQUIVALENCE || RANDOM_EQUIVALENCE) {
                    // Go over to another oracle.
                    disable_regex_oracle = true;
                    continue;
                }

                const std::string exc{e.what()};
                LOG_S(ERROR) << "We got error: " << exc;
                if (exc.find("too complex") != std::string::npos) {
                    // Regex got too big, print and return.
                    LOG_S(INFO) << n_runs << ":END: Regex too big. Printing automaton to " << OUTPUT_FILE;
                    print_current_automaton(my_merger.get(), OUTPUT_FILE, ".partial");
                    return;
                }

                if (exc.find("repeated cex") != std::string::npos) {
                    LOG_S(INFO)
                        << n_runs
                        << ":END: Got repeated counterexample. The equivalence builder sends a default one if all "
                           "regex fails (namely ^(.*)$ from root node.) Write to "
                        << OUTPUT_FILE;
                    print_current_automaton(my_merger.get(), OUTPUT_FILE, ".partial");
                    return;
                }

                throw std::runtime_error("Something wrong equivalence query " + exc);
            }
        }

        ++n_runs;
    }
}

optional<psql::record> ldot_algorithm::equivalence(sqldb_sul_regex_oracle* regex_oracle) {
    if (REGEX_EQUIVALENCE && !disable_regex_oracle)
        return regex_oracle->equivalence_query_db(my_merger.get(), teacher);

    if (RANDOM_EQUIVALENCE) {
        return random_oracle->equivalence_query_db(my_merger.get(), teacher, added_traces);
    }

    if (DISTINGUISHING_EQUIVALENCE) {
        // TODO: Implement this thing.
        // Check for all nodes if their representatives are equivalent using a distinguishing query.
        throw std::runtime_error("Not implemented");
    }
}
