/**
 * @file log_alergia.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-09-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#include "state_merger.h"
#include "evaluate.h"
#include "log_alergia.h"
#include "parameters.h"
#include "input/inputdatalocator.h"

#include <iostream>
#include <unordered_set>

REGISTER_DEF_DATATYPE(log_alergia_data);
REGISTER_DEF_TYPE(log_alergia);


/* void log_alergia_data::initialize() {
    evaluation_data::initialize();
    //symbol_pro.clear();
} */

/* void log_alergia_data::add_tail(tail* t){
} */

/* void log_alergia_data::del_tail(tail* t){
    evaluation_data::del_tail(t);
} */

void log_alergia_data::read_json(json& data){
    evaluation_data::read_json(data);

    json& d = data["trans_probs"];
    for (auto& symbol : d.items()){
        string sym = symbol.key();
        string val = symbol.value();
        normalized_symbol_probability_map[inputdata_locator::get()->symbol_from_string(sym)] = stod(val);
    }

    if(FINAL_PROBABILITIES){
        string fp = data["final_prob"];
        final_prob = stod(fp);
    }

    //log_alergia::normalize_probabilities(this);
};

void log_alergia_data::write_json(json& data){
    evaluation_data::write_json(data);


    for(auto & symbol_count : normalized_symbol_probability_map) {
        int symbol = symbol_count.first;
        double value = symbol_count.second;
        data["trans_probs"][inputdata_locator::get()->string_from_symbol(symbol)] = to_string(value);
    }
    
    if(FINAL_PROBABILITIES) data["final_prob"] = to_string(final_prob);
};

void log_alergia_data::print_transition_label(iostream& output, int symbol){
    output << symbol << ": " << symbol_probability_map[symbol];
};

void log_alergia_data::print_state_label(iostream& output){
    evaluation_data::print_state_label(output);
    output << "Normalized:\n";
    for(auto [symbol, p]: normalized_symbol_probability_map){
        output << symbol << " : " << p << "\n";
    }
    if(FINAL_PROBABILITIES) output << "fP: " << final_prob << "\n";

    output << "Sum:\n";
    for(auto [symbol, p]: symbol_probability_map){
        output << symbol << " : " << p << "\n";
    }
};

/** Merging update and undo_merge routines */

void log_alergia_data::update(evaluation_data* right){
    evaluation_data::update(right);
};

void log_alergia_data::undo(evaluation_data* right){
    evaluation_data::undo(right);
};

/**
 * @brief Finds the max-symbol in this node.
 * 
 * @return int The symbol with the maximum probability.
 */
int log_alergia_data::predict_symbol(tail*){
    double max_p = -1;
    int max_symbol = 0;
    for(const auto &[s, p] : normalized_symbol_probability_map){
        if(max_p < 0 || max_p < p){
            max_p = p;
            max_symbol = s;
        }
    }
    return max_symbol;
};

double log_alergia_data::predict_symbol_score(int t){
    if(t == -1)
        return final_prob;
    else if(normalized_symbol_probability_map.contains(t))
        return normalized_symbol_probability_map[t];
    
    return 0.0;
}

void log_alergia_data::add_probability(const int symbol, const double p) {
    this->symbol_probability_map[symbol] += p;
}

void log_alergia_data::update_probability(const int symbol, const double p) {
    this->symbol_probability_map[symbol] = p;
}

double log_alergia::get_js_term(const double px, const double qx) {
    double term1, term2;
    if(px==0) term1 = 0;
    else term1 = px*log(2*px / (px+qx));

    if(qx==0) term2 = 0;
    else term2 = qx*log(2*qx / (px+qx));

    return 0.5*(term1 + term2);
}

bool log_alergia::consistent(state_merger *merger, apta_node* left_node, apta_node* right_node){
    if(inconsistency_found) return false;
    
    auto* l = static_cast<log_alergia_data*>( left_node->get_data() );
    auto* r = static_cast<log_alergia_data*>( right_node->get_data() );
    unordered_set<int> checked_symbols;

    //cout << "JS divergence: " << get_js_divergence(l->symbol_probability_map, r->symbol_probability_map, l->final_prob, r->final_prob) << ", MU: "<< mu << endl;;

    for(auto& [symbol, left_p] : l->normalized_symbol_probability_map){
        if(!r->normalized_symbol_probability_map.contains(symbol))
            r->normalized_symbol_probability_map[symbol] = 0;

        double right_p = r->normalized_symbol_probability_map[symbol]; // automatically set to 0 if does not contain (zero initialization)
        js_divergence += get_js_term(left_p, right_p);
        if(js_divergence > mu){
            inconsistency_found = true;
            return false;
        }

        checked_symbols.insert(symbol);
    }

    for(auto& [symbol, right_p] : r->symbol_probability_map){
        if(checked_symbols.contains(symbol)) continue;

        if(!l->normalized_symbol_probability_map.contains(symbol))
            l->normalized_symbol_probability_map[symbol] = 0;

        double left_p = l->normalized_symbol_probability_map[symbol]; // automatically set to 0 if does not contain (zero initialization)
        js_divergence += get_js_term(left_p, right_p);
        if(js_divergence > mu){
            inconsistency_found = true;
            return false;
        }
    }

    static auto FINAL_T = 0.3;
    if(FINAL_PROBABILITIES && abs(l->final_prob - r->final_prob) > FINAL_T){
        inconsistency_found = true;
        return false;
    } 


    return true;
};

/* bool log_alergia::consistent(state_merger *merger, apta_node* left_node, apta_node* right_node){
    if(inconsistency_found) return false;
    
    auto* l = static_cast<log_alergia_data*>( left_node->get_data() );
    auto* r = static_cast<log_alergia_data*>( right_node->get_data() );
    unordered_set<int> checked_symbols;

    double nominator=0, d1=0, d2=0;

    for(auto& [symbol, left_p] : l->symbol_probability_map){
        if(!r->symbol_probability_map.contains(symbol))
            r->symbol_probability_map[symbol] = 0;

        nominator += left_p * r->symbol_probability_map[symbol];
        d1 += left_p * left_p;
        d2 += r->symbol_probability_map[symbol] * r->symbol_probability_map[symbol];

        checked_symbols.insert(symbol);
    }

    for(auto& [symbol, right_p] : r->symbol_probability_map){
        if(checked_symbols.contains(symbol)) continue;

        if(!l->symbol_probability_map.contains(symbol))
            l->symbol_probability_map[symbol] = 0;

        nominator += right_p * l->symbol_probability_map[symbol];
        d1 += l->symbol_probability_map[symbol] * l->symbol_probability_map[symbol];
        d2 += right_p * right_p;
    }

    js_divergence = nominator / (sqrt(d1) * sqrt(d2));

    if(js_divergence < mu){
        inconsistency_found = true;
        return false;
    }

    // weights for probabilistic machines can be dominated by final_ps, leading to merging everything, hence we do the check double
    if(FINAL_PROBABILITIES){
        nominator += l->final_prob * r->final_prob;
        d1 += l->final_prob * l->final_prob;
        d2 += r->final_prob * r->final_prob;
        js_divergence = nominator / (sqrt(d1) * sqrt(d2));

        cout << "js2 " << js_divergence << endl;

        if(js_divergence < mu){
            inconsistency_found = true;
            return false;
        }
    }

    return true;
}; */

double log_alergia::get_js_divergence(unordered_map<int, double>& left_distribution, unordered_map<int, double>& right_distribution, double left_final, double right_final){
    double res = FINAL_PROBABILITIES ? 0 : get_js_term(left_final, right_final);
    unordered_set<int> checked_symbols;
    for(auto& [symbol, left_p] : left_distribution){
        double right_p = right_distribution[symbol]; // automatically set to 0 if does not contain (zero initialization)
        auto js = get_js_term(left_p, right_p);
        res += js;
        
        checked_symbols.insert(symbol);
    }

    for(auto& [symbol, right_p] : right_distribution){
        if(checked_symbols.contains(symbol)) continue;
        
        double left_p = left_distribution[symbol]; // automatically set to 0 if does not contain (zero initialization)
        auto js = get_js_term(left_p, right_p);
        res += js;
    }

    return res;
}

/**
 * @brief Normalizes the symbol probability map, and stores it.
 * 
 * @param node The node.
 */
void log_alergia::normalize_probabilities(log_alergia_data* data) {
    double p_sum = 0;
    for(auto& [symbol, p] : data->symbol_probability_map){
        p_sum += p;
    }

    assert(p_sum != 0);

    double factor = FINAL_PROBABILITIES ? (1. - data->final_prob) / p_sum : 1 / p_sum;
    //double factor = 1 / p_sum;
    assert(factor >= 0);

    for(auto& [symbol, p] : data->symbol_probability_map){
        data->normalized_symbol_probability_map[symbol] = p * factor;
    }
}

double log_alergia::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    return js_divergence;
};

void log_alergia::reset(state_merger *merger){
    inconsistency_found = false;
    js_divergence = 0;
};

void log_alergia::initialize_before_adding_traces() {
    mu = MU;
}