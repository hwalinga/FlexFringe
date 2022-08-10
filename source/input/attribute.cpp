#include <sstream>
#include "attribute.h"

using namespace std;

/* attribute constructor from string
 * if it contains d -> attribute is discrete
 * if it contains s -> attribute can be used to infer guards
 * if it contains f -> attribute is a distribution variable
 * if it contains t -> attribute is a target variable
 * */
Attribute::Attribute(const string& input){
    discrete = false;
    splittable = false;
    distributionable = false;
    target = false;

    stringstream cs(input);
    string attr_name;
    string attr_types;
    std::getline(cs,attr_name, '=');
    std::getline(cs,attr_types);

    if(attr_types.find('d') != std::string::npos) discrete = true;
    if(attr_types.find('s') != std::string::npos) splittable = true;
    if(attr_types.find('f') != std::string::npos) distributionable = true;
    if(attr_types.find('t') != std::string::npos) target = true;

    name = attr_name;

    cs.clear();
}