#ifndef cornus_thicket_expressions_hpp
#define cornus_thicket_expressions_hpp

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace cornus_thicket {

struct Var{
    const std::string vname;
    std::optional<std::string> value;
};

struct VarPool:
        public std::map<std::string, Var>
{
    void putVar(const Var& var){
        (*this)[var.vname];
    }

    const Var* findVar(const std::string& vn) const {
        auto found = find(vn);
        if(found != this->end()){
            return &found->second;
        }else{
            return nullptr;
        }
    }
};


struct VarOccurrence{
    const std::string vname;
    const size_t place_pos; // where encountered
    const size_t place_length;

    std::optional<std::string> value;

    VarOccurrence(const std::string& where, size_t pos, size_t length)
        : vname(where.substr(pos, length)), place_pos(pos), place_length(length)
    {}

    static
    std::string // error string or empty
    parse(const std::string& s, std::vector<VarOccurrence>& output_vars){
        using std::string;
        for(size_t pos = 0; pos != string::npos; ){
            size_t vpos = s.find("${", pos);
            if(vpos ==  string::npos){
                break;
            }

            size_t vendpos = s.find("}", pos);

            if(vendpos == string::npos){
                return "Variable syntax error: no closing }";
            }

            size_t place_length = vendpos - vpos + 1;
            VarOccurrence vocc(s.substr(vpos + 2, place_length - 3), vpos, place_length);


            output_vars.push_back(std::move(vocc));

            pos = vendpos + 1;
        }

        return string();
    }

};

template<typename ErrHandler>
std::string  substituteExpressions(
        const VarPool& vpool,
        const std::string s,
        ErrHandler& err_handler
){
    std::vector<VarOccurrence> voccs;
    auto err = VarOccurrence::parse(s, voccs);

    if(!err.empty()){ // if syntax errors
        err_handler(err);
        return std::string();
    }

    if(voccs.empty()){
        return s;  // no expressions inside the string
    }

    std::string ret;
    size_t cur_text_pos = 0;
    for(auto& vocc: voccs){
        //texts.push_back(s.substr(cur_text_pos, vocc.place_pos - cur_text_pos));
        ret += s.substr(cur_text_pos, vocc.place_pos - cur_text_pos);

        const Var* pvar = vpool.findVar(vocc.vname);

        if(pvar == nullptr){
            err_handler(std::string("Variable not found: " + vocc.vname) );
        }else if(!pvar->value) {
            err_handler(std::string("Variable is not set: " + vocc.vname) );
        }else{
            ret += pvar->value.value();
        }

        cur_text_pos = vocc.place_pos + vocc.place_length;
    }

    auto& last_vocc = voccs[voccs.size() - 1];

    ret += s.substr(last_vocc.place_pos + last_vocc.place_length); // last text chunk after last variable
    return ret;
}

}


#endif
