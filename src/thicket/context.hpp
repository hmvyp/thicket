#ifndef ftreeimporter_context_hpp
#define ftreeimporter_context_hpp

#include "node.hpp"
#include <streambuf>
#include <fstream>
#include <cctype>
#include <algorithm>

namespace ftreeimporter {

#define THICKET_MOUNT_SUFFIX_STR(literal_prefix) literal_prefix##".thicket_mount.txt"

// string conversions:

template<typename some_string_t>
const some_string_t
string2some_string(const std::string& s);

template<>
inline
const std::wstring
string2some_string<std::wstring>(const std::string& s){
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt;  // Deprecated?
    return cvt.from_bytes(s);
}

template<>
inline
const std::string
string2some_string<std::string>(const std::string& s){ return s;}

inline
string_t
string2path_string(const std::string& s){ return string2some_string<string_t>(s); }


// trim string:
inline
std::string
trim(const std::string& s)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),[](unsigned char c){return std::isspace(c);});
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](unsigned char c){return std::isspace(c);}).base();
   return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}


struct Context
        : public ObjectFactory
{
    template<typename CharT>
    constexpr const CharT* MNT_SUFFIX_T(){
        if constexpr (sizeof(CharT) == sizeof(char)) {
            return THICKET_MOUNT_SUFFIX_STR();
        }else{
            return THICKET_MOUNT_SUFFIX_STR(L);  //wide character literal
        }
    };

    constexpr const char_t* MNT_SUFFIX(){return MNT_SUFFIX_T<char_t>();}


    fs::path root_;  // converted to canonical
    fs::path scope_; // converted to canonical

    std::map<fs::path, Node*> nodes;

    Context(
            fs::path root, // the whole universe
            fs::path scope // a folder where imports shall be resolved (path relative to root)
    ){

        std::error_code err;

        root_ = fs::canonical(root, err);
        if(!err){
            report_error("ftreeimporter::Context: Invalid root path", SEVERITY_PANIC);
        }

        scope_ =  fs::canonical(root_/scope, err);

        if(!err){
            report_error("ftreeimporter::Context: Invalid scope path", SEVERITY_PANIC);
        }

        // ToDo: make root and scope nodes

        if(existingFileAt(root_) == nullptr){
            report_error("ftreeimporter::Context: Invalid root path", SEVERITY_PANIC);
        }

        if(existingFileAt(scope_) == nullptr){
            report_error("ftreeimporter::Context: Invalid scope path", SEVERITY_PANIC);
        }

    }

    Node* existingFileAt(const fs::path& p){
        auto it = nodes.find(p);
        if(it != nodes.end()){
            return it->second;
        }

        Node* nd = nullptr;

        fs::file_status fstat = status(p);

        switch(fstat.type()){
        case fs::file_type::regular:
            nd = this->create<Node>(p);
            nd->target_type = FILE_NODE;
            break;
        case fs::file_type::directory:
            nd = this->create<Node>(p);
            nd->target_type = DIR_NODE;
            break;
        default:
            // report_error(std::string("ftreeimporter: file does not exist: ") + p.string(), SEVERITY_ERROR);
            return nullptr;
        }

        nd->ref_type = FINAL_NODE;
        nd->valid_= true;

        nodes[p] = nd;
        return nd;
    }

    Node* mountpointAt(const fs::path& p){
        if(p.begin() == p.end()){
            return nullptr;
        }

        auto pm = p;

        pm.replace_filename( (string_t)p.filename() + MNT_SUFFIX() );

        fs::file_status fstat = status(pm);

        if(fstat.type() != fs::file_type::regular){
            return nullptr; // not a mountpoint (net necessary an error)
        }

        // The path represents a mountpoint.
        // If something goes wrong it is an error

        Node* nd = create<Node>(p);
        auto& mt = nd->mount_targets;

        try{
            std::ifstream is(pm);
            std::stringstream buffer;
            buffer << is.rdbuf();

            std::string line;
            while(buffer >> line) {
                auto text_path = trim(line);
                if(text_path.empty()){
                    continue;
                }

                mt.push_back(std::move(text_path));
            }
        }catch(...){
            report_error( std::string("can not read mount point at ") + pm.c_str(), SEVERITY_ERROR);
            return nullptr;
        }

        for(auto it = mt.begin(); it != mt.end() ; it++ ){

            auto pas = string2path_string(*it);
            fs::path p = fs::path(pas);

            Node* tgn = resolve(p);
            if(tgn == 0){
                report_error( std::string("can not read mount point at ") + pm.c_str(), SEVERITY_ERROR);
                continue;
            }

            nd->targets.push_back(tgn);
        }

        return nullptr; // duck!!!
    }


    Node* nodeAt(const fs::path& p){ // assume p canonical
        auto it = nodes.find(p);
        if(it != nodes.end()){
            return it->second;
        }

        auto parp = p.parent_path();
        if(p.empty() || p.begin() == p.end() || parp.empty() || parp == p){
            report_error(std::string("ftreeimporter: nodeAt() ::Parent path unavailable for ") + p.string(), SEVERITY_ERROR);
            return nullptr;
        }

        Node* parn = nodeAt(parp);
        if(!parn){
            return nullptr;
            // ToDo: report error?
        }

        if(parn->resolved){
            auto last = -- p.end();  // (p.begin() != p.end() already checked )
            auto& c =  parn->children;
            auto it = c.find(*last);

            if(it == c.end()){
                return nullptr;
            }else{
                return it->second;
            }
        }

        // else try as mountpoint and resolve it; if no, try filesystem files under parent
        // mountpoint has priority since filesystem folder can be a generated one.
        Node* nd = mountpointAt(p);

        if(nd) {
            return nd;
        }

        return existingFileAt(p);
    }

    Node* resolve(const fs::path& p){
        Node* nd = nullptr;
        auto it = nodes.find(p);

        if(it == nodes.end()){
            nd = create<Node>(p);
            nodes[p] = nd;
        }else{
            nd = it->second;
        }

        if(nd->resolved){
            return nd;
        }

        fs::file_status fstat = status(p);

        if(nd->ref_type == UNKNOWN_REFTYPE) {
            auto mnt_description_path = p / MNT_SUFFIX();
            //auto stat =
            // ToDo: ...

        }

        return nullptr; // ToDo: ...

        //....

    }
};


} // namespace

#endif
