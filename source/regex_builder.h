/**
 * @file regex_builder.h
 * @author Hielke Walinga (hielkewalinga@gmail.com)
 * @brief For building regex with a DFA.
 * @version 0.1
 * @date 2024-1-9
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef __REGEX_BUILDER_H__
#define __REGEX_BUILDER_H__

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "apta.h"
#include "state_merger.h"


class regex_builder{
public:

    /**
     * @brief Builds a class that for every output symbol constructs a regex to mach strings that output that output symbol.
     *
     * Please note that symbols by apta are internally hold as single points.
     * To keep this in the regex, the single points must be converted to single characters.
     * Provide a lambda or a mapping that can convert this. // TODO: override this somehow.
     *
     * This would mean that a regex conversion for how traces are usually encoded will only 
     * work when its alphabet only consists of single letter symbols (ie 0-9).
     * For a larger alphabet, you could for example use the (also limited) ascii conversions
     * from sqldb.cpp. Which increases it to about some more than 50.
     *
     * TODO: You can extend its range with UTF8.
     * See: https://stackoverflow.com/questions/26074090/iterating-through-a-utf-8-string-in-c11
     */
    
    explicit regex_builder(apta& the_apta, state_merger& merger, std::tuple<bool, bool, bool>& coloring, std::unordered_map<int, char>& mapping);
    explicit regex_builder(apta& the_apta, state_merger& merger, std::tuple<bool, bool, bool>& coloring, const std::function<char(int)>& mapping_func);

    void initialize(apta& the_apta, state_merger& merger, std::tuple<bool, bool, bool>& coloring, const std::function<char(int)>& mapping_func);

    apta_node* root;

    // States of the DFA
    std::unordered_set<apta_node*> states;
    // Transitions in the DFA
    std::unordered_map<apta_node*, std::unordered_map<apta_node*, char>> transitions;
    // Reverse transitions in the DFA
    std::unordered_map<apta_node*, std::unordered_set<apta_node*>> r_transitions;

    // Predicted types of each state
    std::unordered_map<std::string, std::vector<apta_node*>> types_map;


    // The transition to the final state is empty, we use the "audible bell" character to indicate this.
    // as I hope that is never a real symbol. Another possible char is '\0', but I don't know
    // if other weird convetions can break that, because '\0' is the default when char is initialized.
    char EMPTY = '\a';

    /**
     * @brief Convert the APTA to a regex string.
     *
     * This function converts the structure to of the APTA to a regex.
     * This is inspired by the implementation found here:
     * https://github.com/caleb531/automata/blob/c39df7d588164e64a0c090ddf89ab5118ee42e47/automata/fa/gnfa.py#L345
     *
     * Argument is the external string representation.
     */
    std::string to_regex(std::string output_state);
    std::string to_regex(int output_state);

    /**
     * @brief Check if the regex string needs to bracketed.
     * If a | is not inside existing brackets, it should be brackets.
     */
    bool brackets(const std::string& regex);
};

#endif
