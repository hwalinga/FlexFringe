//
// Created by tom on 1/6/23.
//

#ifndef FLEXFRINGE_SYMBOL_INFO_H
#define FLEXFRINGE_SYMBOL_INFO_H


#include <vector>
#include <unordered_map>
#include <string>

/**
 * This class represent one input symbol, read from an input source.
 * It should contain all the information necessary to later on be turned into
 * tails and traces in some inputdata instance.
 */
class symbol_info {
private:
    std::unordered_map<std::string, std::vector<std::string>> properties;
public:
    void set(const std::string& name, const std::vector<std::string>& property_list);
    void set(const std::string& name, const std::string& property);

    const std::vector<std::string>& get(const std::string& name);
    std::string get_str(const std::string& name);

    bool has(const std::string& name);
};


#endif //FLEXFRINGE_SYMBOL_INFO_H
