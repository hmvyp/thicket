#ifndef cornus_thicket_filter_hpp
#define cornus_thicket_filter_hpp

#include <string>
#include <regex>


namespace cornus_thicket {


struct FilterMatch
{
    bool matches; // true on success
    bool no_recurse; // optimization hint in case of matches == false
};


struct StrFilter
{
    virtual std::string // returns error string
    init(const strview_type& filter_str) = 0;

    virtual FilterMatch
    match(
            // s is a relative path to match against the filter (relative to filtering root).
            // It starts from a last slash (including the slash!) before wildcards.
            // For filtering root itself (absolute path until the last slash) s is empty.
            const strview_type& s,
            bool is_directory // (additional info)
    ) = 0;

    virtual ~StrFilter(){}
};


struct GlobFilter
: StrFilter
{
    // common pattern form: [**/]pref[*suff]
    // [...] means optional
    // pref and suff can be empty
    virtual std::string // returns error string
    init(const strview_type& filter_str) override {
        static const std::string recurse_pattern("**/");
        static const std::string universal_pattern("**/*");

        if((strview_type)universal_pattern == filter_str){
            is_universal = true;
            return std::string();
        }

        auto mmres = std::mismatch(
                recurse_pattern.begin(), recurse_pattern.end(),
                filter_str.begin(), filter_str.end()
        );

        size_t start_match_pos = 0;

        if(mmres.first == recurse_pattern.end()) { // if recursive pattern presents
            start_match_pos = recurse_pattern.size();
            is_recursive = true;
        }

        // matching pattern pref[*suff]:
        strview_type mp = filter_str.substr(start_match_pos);

        size_t ast_pos = mp.find_first_of('*');

        if(ast_pos != strview_type::npos){
            if(ast_pos != mp.find_last_of('*')) {
                return std::string("syntax error in glob pattern: extra asterisk found");
            }

            has_asterisk = true;

            pref = std::string(mp.begin(), mp.begin() + ast_pos);
            suff = std::string(mp.begin() + ast_pos + 1, mp.end());
        }else{
            pref = std::string(mp.begin(), mp.end());
        }

        if(pref.substr(0,1) != "/"){
            pref = std::string("/") + pref; // prevent breaking change in "*" filter interpretation
        }

        return std::string(); // Ok
    }

    virtual FilterMatch
    match(
            const strview_type& sv, // relative path to match against the filter
            bool is_directory
    ) override {
        if(is_universal){
            return FilterMatch{true};
        }

        size_t match_start_pos = 0;

        size_t last_slash_pos = sv.find_last_of('/');

        if(is_recursive){
            if(last_slash_pos != strview_type::npos){ // true except for matching root
                match_start_pos = last_slash_pos;
            }
        }

        if(is_directory){ //ToDo: maybe allow directory matching using patterns trailing with a slash?
            return FilterMatch{false, (!is_recursive) && !sv.empty()}; // (always allow recursion for filtering root)
        }

        strview_type to_match = sv.substr(match_start_pos);

        if(
                pref.size() + suff.size() >  to_match.size()
                ||
                to_match.substr(0, pref.size()) != strview_type(pref)
                ||
                to_match.substr(to_match.size() - suff.size()) != strview_type(suff)
        ){
            return FilterMatch{false, false}; // ToDo: maybe count slashes to calc no_recurse in case of !is_recursive?
        }

        return FilterMatch{true};
    }


protected:
    bool is_universal = false;
    bool is_recursive = false;  // true if "**/" presents
    bool has_asterisk = false;  // true if "*" (single asterisk) presents
    std::string pref;   // before asterisk
    std::string suff;   // after_asterisk
};


struct RegexFilter
        : public StrFilter
{
    virtual std::string // returns error string
    init(const strview_type& filter_str) override {
        rgx_text = std::string(filter_str.begin(), filter_str.end());

        try{
            rgx.assign(
                    rgx_text,
                    std::regex::extended // syntax closer to re2 as potential replacement for std:regex
                    | std::regex::nosubs
                    | std::regex::optimize
            );
        }catch(...){
            return std::string("exception while constructing regular expression: ") + rgx_text;
        }

        valid = true;

        return std::string(); // ok
    }

    virtual FilterMatch
    match(
            const strview_type& s,
            bool is_directory
    ) override {
        if(!valid){
            return FilterMatch{false, true};
        }

        if(is_directory){
            return FilterMatch{false};
        }

        bool res =regex_match(s.begin(), s.end(), rgx);
        return FilterMatch{res};
    }

protected:
    bool valid = false;
    std::string rgx_text;
    std::regex rgx; //(".*(a|xayy)", std::regex::extended); // POSIX
};




struct Filter
{
    std::string // returns error str
    setFilter(const std::string& filter_str){
        static const std::string rgx_marker("***");

        auto mmres = std::mismatch(
                rgx_marker.begin(), rgx_marker.end(),
                filter_str.begin(), filter_str.end()
        );

        if(mmres.first == rgx_marker.end()){ // regex case
            StrFilter* pf = new RegexFilter();
            auto errstr = pf->init(
                    strview_type(
                            filter_str.c_str() + rgx_marker.size()
                    )
            );
            pfilter.reset(pf);
            return errstr;
        }else{ // Glob case
            StrFilter* pf = new GlobFilter();
            auto errstr = pf->init(filter_str);
            pfilter.reset(pf);
            return errstr;
        }
    }

    FilterMatch
    match(const strview_type& sv, bool is_directory){
        return pfilter->match(sv, is_directory);
    }


private:
    std::unique_ptr<StrFilter> pfilter;
};


} // namespace

#endif
