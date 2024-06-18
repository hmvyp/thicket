#ifndef cornus_thicket_expressions_hpp
#define cornus_thicket_expressions_hpp

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace cornus_thicket {

struct Var{

    std::string vname;

    bool is_set = false;

    void setValue(const std::string& val){
        is_set = true;
        value_ = val;
    }

    std::string getValue() const { return value_;}


    std::string value_;
};

struct VarPool:
        public std::map<std::string, Var>
{
    void putVar(const Var& var){
        (*this)[var.vname] = var;
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
        : vname(where.substr(pos + 2, length - 3))
        , place_pos(pos)
        , place_length(length)
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

            VarOccurrence vocc(s, vpos, place_length);

            output_vars.push_back(vocc);

            pos = vendpos + 1;
        }

        return string();
    }

};

template<typename ErrHandler>
inline
std::string  substituteExpressions(
        const VarPool& vpool,
        const std::string& s,
        const ErrHandler& err_handler // returns true to cancel substitution
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
        ret += s.substr(cur_text_pos, vocc.place_pos - cur_text_pos);

        const Var* pvar = vpool.findVar(vocc.vname);

        if(pvar == nullptr){
            if(err_handler(std::string("Variable not found: " + vocc.vname) )){
                return std::string();
            }
        }else if(!pvar->is_set) {
            if(err_handler(std::string("Variable is not set: " + vocc.vname) )){
                return std::string();
            }
        }else{
            ret += pvar->getValue();
        }

        cur_text_pos = vocc.place_pos + vocc.place_length;
    }

    auto& last_vocc = voccs[voccs.size() - 1];

    ret += s.substr(last_vocc.place_pos + last_vocc.place_length); // last text chunk after last variable
    return ret;
}


inline
std::string // error string
addVarOptions(VarPool& add_here, const std::vector<const char* >& varoptions){
    using std::string;
    for(auto optval: varoptions){
        string ov(optval);
        size_t colon_pos = ov.find(':');
        if(colon_pos == string::npos){
            return string("-var option shall contain colon ':' between variable name and value, but it does not: ") + ov;
        }

        Var var;
        var.vname =ov.substr(0, colon_pos);
        var.setValue(ov.substr(colon_pos + 1));

        add_here.putVar(var);
    }

    return string();
}

}


#endif
