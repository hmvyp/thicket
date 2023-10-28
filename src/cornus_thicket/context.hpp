#ifndef cornus_thicket_context_hpp
#define cornus_thicket_context_hpp

#include "node.hpp"
#include <streambuf>
#include <fstream>
#include <cctype>
#include <algorithm>

namespace cornus_thicket {

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
            report_error("cornus_thicket::Context: Invalid root path", SEVERITY_PANIC);
        }

        scope_ =  fs::canonical(root_/scope, err);

        if(!err){
            report_error("cornus_thicket::Context: Invalid scope path", SEVERITY_PANIC);
        }

        // ToDo: make root and scope nodes

        if(existingFileAt(root_) == nullptr){
            report_error("cornus_thicket::Context: Invalid root path", SEVERITY_PANIC);
        }

        if(existingFileAt(scope_) == nullptr){
            report_error("cornus_thicket::Context: Invalid scope path", SEVERITY_PANIC);
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
            // report_error(std::string("cornus_thicket: file does not exist: ") + p.string(), SEVERITY_ERROR);
            return nullptr;
        }

        nd->ref_type = FINAL_NODE;
        nd->valid_= true;

        nodes[p] = nd;
        return nd;
    }

    // also resolves the reference node:
    Node* mountpointAt(const fs::path& p){
        if(p.begin() == p.end()){
            return nullptr;
        }

        auto pm = p;  // will be path to mountpoint description file

        pm.replace_filename( (string_t)p.filename() + MNT_SUFFIX() );

        fs::file_status fstat = status(pm);

        if(fstat.type() != fs::file_type::regular){
            return nullptr; // not a mountpoint (not necessary an error)
        }

        // The path represents a mountpoint.
        // From here, if something goes wrong, it is an error

        Node* nd = create<Node>(p);
        nd->ref_type = REFERENCE_NODE;
        auto& mt = nd->mount_targets;

        try{ // read mountpoint description file:
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
            nd->resolved_ = NODE_FAILED_TO_RESOLVE;
            report_error( std::string("can not read mount point at ") + pm.c_str(), SEVERITY_ERROR);
            return nd;
        }

        // common error prefix lambda:
        auto erprfx = [&pm](const std::string& mnt_entry) -> std::string {
            return  std::string("mountpoint target ") + mnt_entry +
                 " in mountpoint description file: " + pm.c_str();
        };

        // run over mountpoint entries:
        for(auto it = mt.begin(); it != mt.end() ; it++ ){

            auto& en = *it; // entry

            bool from_root = (en[0] == '/') ? true : false; // path from the root ("absolute") or relative to mountpoint

            if(from_root){
                en.erase(0,1); // remove "absolute" mark
            }

            auto pas = string2path_string(en); // entry as relative native string
            auto prt = fs::path(pas); // the entry as fs:path

            if(prt.empty()){
                report_error(erprfx(pm) + "results in empty path", SEVERITY_ERROR);
                continue;
            }

            fs::path pt;  // target path

            if(from_root) {
                pt = this->root_ / prt;
            }else{
                pt = p.parent_path() / prt;
            }

            std::error_code err;
            auto ptcn = fs::weakly_canonical(pt, err); // the tail may not exist (e.g. may point to another mountpoint)
            if(err){
                report_error(erprfx(pm) + "mountpoint target "
                        + pt.c_str() + " can not be converted to canonical path"
                        , SEVERITY_ERROR
                );
                continue;
            }

            Node* tgn = resolveAt(ptcn); // resolve the target
            if(tgn == 0){
                report_error( erprfx(pm) +
                            + " can not resolve mountpoint target:"
                            + ptcn.c_str()
                        , SEVERITY_ERROR
                );
                continue;
            }

            nd->targets.push_back(tgn);
        }

        nd->ref_type = REFERENCE_NODE;
        if(!nd->targets.empty()) {
            nd->target_type = nd->targets[0]->target_type; // ToDo  in resolve: check if regular file, it shall be the same for all targets
        }

        nd->valid_ = true;

        // Reference node is useless being unresolved, so resolve it:
        resolveReference(*nd);

        return nd;
    }


    Node* nodeAt(const fs::path& p){ // assume p is canonical
        auto it = nodes.find(p);
        if(it != nodes.end()){
            return it->second; // node already exists
        }

        auto parp = p.parent_path();
        if(p.empty() || p.begin() == p.end() || parp.empty() || parp == p){
            report_error(std::string("cornus_thicket: nodeAt() ::Parent path unavailable for ") + p.string(), SEVERITY_ERROR);
            return nullptr;
        }

        Node* parn = nodeAt(parp);
        if(!parn){
            return nullptr; // no parent
            // ToDo: report error?
        }

        if(parn->resolved_ == NODE_RESOLVED){
            auto last = -- p.end();  // (p.begin() != p.end() already checked )
            auto& c =  parn->children;
            auto it = c.find(*last);

            if(it == c.end()){
                return nullptr;
            }else{
                return it->second;
            }
        }else if(
                parn->ref_type == REFERENCE_NODE
                && parn->resolved_ == NODE_FAILED_TO_RESOLVE
        ){
            return nullptr;
        }

        // ... else try as mountpoint. If failed, try as filesystem object under the parent.
        // Mountpoint has priority over filesystem since filesystem object can be a generated one.
        Node* nd = mountpointAt(p);

        if(nd) {
            return nd;
        }

        return existingFileAt(p);
    }

    Node* resolveAt(const fs::path& p){
        Node* nd = nodeAt(p);

        if(nd == nullptr || nd->resolved_ >= NODE_RESOLVED){
            return nd;
        }

        resolve(*nd);
        return nd;
    }

    void resolve(Node& n){ // assuming reference type is set for n
        if(n.resolved_ >= NODE_RESOLVED){
            return;
        }

        try{
            switch(n.ref_type){
            case FINAL_NODE:
                resolveFinal(n);  // resolve as filesystem object
                return;
            case REFERENCE_NODE:
                resolveReference(n);  // resolve as mountpoint or its descendants
                return;
            default:
                return; // unresolved
            }
        }catch(...){
        }
    }

    void resolveFinal(Node& n);

    void resolveReference(Node& n);

private:
    void collectRefnodeChildren(Node& n);

};

} // namespace

#include "context_resolve_ref.hpp"

#endif
