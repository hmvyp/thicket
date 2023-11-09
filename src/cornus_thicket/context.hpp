#ifndef cornus_thicket_context_hpp
#define cornus_thicket_context_hpp

#include "base.hpp"
#include "utils.hpp"

#include "node.hpp"
#include <streambuf>
#include <fstream>

#include <cctype>
//#include <algorithm>
//#include <codecvt>


namespace cornus_thicket {

// #define THICKET_MOUNT_SUFFIX_STR(literal_prefix) literal_prefix##".thicket_mount.txt"
#define THICKET_MOUNT_SUFFIX_STR(literal_prefix) CORNUS_THICKET_CONCAT(literal_prefix,CORNUS_THICKET_MOUNTPOINT_SUFFIX)



struct Context
        : public ObjectFactory
{
    template<typename CharT>
    static constexpr const CharT* MNT_SUFFIX_T(){
        if constexpr (sizeof(CharT) == sizeof(char)) {
            return THICKET_MOUNT_SUFFIX_STR();
        }else{
            return THICKET_MOUNT_SUFFIX_STR(L);  //wide character literal
        }
    };

    static constexpr const char_t* MNT_SUFFIX(){return MNT_SUFFIX_T<char_t>();}

    static inline size_t MNT_SUFFIX_LENGTH = std::strlen(THICKET_MOUNT_SUFFIX_STR());


    fs::path root_;  // converted to canonical
    fs::path scope_; // converted to canonical

    std::map<fs::path, Node*> nodes;

    Context(
            fs::path root, // the whole universe
            fs::path scope // a folder where imports shall be resolved (path relative to root)
    ){

        std::error_code err;

        root_ = fs::canonical(root, err);
        if(err){
            report_error("cornus_thicket::Context: Invalid root path", SEVERITY_PANIC);
        }

        scope_ =  fs::canonical(root_/scope, err);

        if(err){
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

    bool path_in_scope(const fs::path& p_canon){
       const char_t* ss = scope_.c_str();
       const char_t* ps = p_canon.c_str();

       for(int i = 0;; ++i){
           if(ss[i] == 0){
               return true;
           }else if(ss[i] != ps[i]){
               break;
           }
       }
       return false;
    }

    Node* existingFileAt(const fs::path& p){
        fs::file_status fstat = fs::symlink_status(p);

        // The path represents a filesystem object.
        // From here, if something goes wrong, it is an error
        // and we shall return erroneous node instead of nullptr

        Node* nd = this->create<Node>(p);
        nd->ref_type = FINAL_NODE;
        nodes[p] = nd;

        if(fs::is_symlink(fstat) || fs::is_regular_file(fstat)){
            nd->target_type = FILE_NODE;
        }else if(fs::is_directory(fstat)){
            nd->target_type = DIR_NODE;
        }else{
            report_error(std::string("cornus_thicket: file type unsupported (shall be regular, symlink or directory) ")
                    + p.string()
                    , SEVERITY_ERROR
            );
            return nd; // return invalid node
        }

        nd->valid_= true;
        return nd;
    }

    // also resolves the reference node:
    Node* mountpointAt(const fs::path& p){
        if(p.begin() == p.end()){
            return nullptr;
        }

        auto pm = p;  // will be path to mountpoint description file

        pm.replace_filename( (string_t)p.filename() + MNT_SUFFIX() );

        fs::file_status fstat = symlink_status(pm);

        if(fstat.type() != fs::file_type::regular){
            return nullptr; // not a mountpoint (not necessary an error)
        }

        // The path represents a mountpoint.
        // From here, if something goes wrong, it is an error
        // and we shall return erroneous node instead of nullptr

        Node* nd = create<Node>(p);
        nd->ref_type = REFERENCE_NODE;
        nodes[p] = nd;

        auto& mt = nd->mount_targets;

        try{ // read mountpoint description file:
            std::ifstream is(pm);
            std::stringstream buffer;
            buffer << is.rdbuf();

            std::string line;
            while(std::getline(buffer, line)) {
                auto text_path = trim(line);
                if(text_path.empty() || text_path[0] == '#'){ // emty or comment
                    continue;
                }

                mt.push_back(std::move(text_path));
            }
        }catch(...){
            nd->resolved_ = NODE_FAILED_TO_RESOLVE;
            report_error( std::string("can not read mount point description at ") + p2s(pm), SEVERITY_ERROR);
            return nd;
        }

        // common error prefix lambda:
        auto erprfx = [&pm](const std::string& mnt_entry) -> std::string {
            return
                 std::string("\n    in mountpoint description file:\n    ") + p2s(pm)
                 + "\n    mountpoint target:\n    " + mnt_entry
                 + "\n    ";
        };

        // run over mountpoint entries to calculate and resolve targets:
        for(auto& eno : mt){
			std::string en = eno; // copy as en may be changed (leave original untouched)
			
            bool from_root = (en[0] == '/') ? true : false; // path from the root ("absolute") or relative to mountpoint

            if(from_root){
                en.erase(0,1); // remove "absolute" mark
            }

            auto pas = string2path_string(en); // entry as relative native string
            auto prt = fs::path(pas); // the entry as fs:path

            if(prt.empty()){
                nd->resolved_ = NODE_FAILED_TO_RESOLVE;
                report_error(erprfx(eno) + "results in empty path", SEVERITY_ERROR);
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
                nd->resolved_ = NODE_FAILED_TO_RESOLVE;
                report_error(erprfx(eno) + "mountpoint target\n    "
                        + p2s(pt) + "\n    can not be converted to canonical path"
                        , SEVERITY_ERROR
                );
                continue;
            }

            Node* tgn = resolveAt(ptcn); // resolve the target
            if(tgn == 0){
                nd->resolved_ = NODE_FAILED_TO_RESOLVE;
                report_error( erprfx(eno) +
                            + "can not resolve mountpoint target:\n    "
                            + p2s(ptcn)
                        , SEVERITY_ERROR
                );
                continue;
            }

            nd->targets.push_back(tgn);
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

    Node* resolve(){
        return resolveAt(this->scope_);
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

    // materialization methods:
    void clean(); // cleans all under scope
    void materializeAsSymlinks(); // materializes all under scope

    bool silent_ = false;
    bool force_ = false;

private:
    void collectRefnodeChildren(Node& n);
    static bool is_thicket_mountpoint_description(const fs::path& p, fs::path* mountpoint_path);

    static void clean(const fs::path& p, std::map<fs::path, bool>& to_delete);
    void materializeAsSymlinks(Node& n);
};

} // namespace

#include "context_resolve_fin.hpp"
#include "context_resolve_ref.hpp"
#include "context_materialize_symlinks.hpp"

#endif
