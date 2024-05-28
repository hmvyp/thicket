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

#define THICKET_MOUNT_SUFFIX_STR(literal_prefix) CORNUS_THICKET_CONCAT(literal_prefix,CORNUS_THICKET_MOUNTPOINT_SUFFIX)


struct MountRecord; //forward

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
            report_error(
                    std::string("cornus_thicket::Context: Invalid root path")+ p2s(root),
                    SEVERITY_PANIC
            );
        }

        scope_ =  fs::canonical(root_/scope, err);

        if(err){
            report_error(
                    std::string("cornus_thicket::Context: Invalid scope path") + p2s(scope),
                    SEVERITY_PANIC
            );
        }

        mkRootAndScopeNodes();
    }

    Context(
            unsigned root_level, // number of levels above the scope
            fs::path scope  // in this case: absolute or relative to the current directory
    ){

        std::error_code err;

        scope_ =  fs::canonical(scope, err);

        if(err){
            report_error(
                    std::string("cornus_thicket::Context: Invalid scope path") + p2s(scope),
                    SEVERITY_PANIC
            );
        }

        fs::path root = scope_;

        for(unsigned i =  root_level; i != 0; i--){
            root = root.parent_path();
        }

        root_ = root;

        mkRootAndScopeNodes();
    }

private:
    void mkRootAndScopeNodes(){ // this is actually just a part of ctors
        if(existingFileAt(root_) == nullptr){
            report_error("cornus_thicket::Context: Invalid root path", SEVERITY_PANIC);
        }

        if(existingFileAt(scope_) == nullptr){
            report_error("cornus_thicket::Context: Invalid scope path", SEVERITY_PANIC);
        }
    }

public:
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

        NodeType tt = UNKNOWN_NODE_TYPE;

        if(fs::is_symlink(fstat) || fs::is_regular_file(fstat)){
            tt = FILE_NODE;
        }else if(fs::is_directory(fstat)){
            tt = DIR_NODE;
        }else{

            return nullptr;  // silently skip exotic filesystem objects
        }

        // from here it seems that it is final node
        // (we hope the caller previously checks if it is a mountpoint)

        auto it = nodes.find(p);
        if(it != nodes.end()){
            Node* ret = it->second; // node already exists
            return ret; // maybe check if node is final and report an internal error otherwise?
        }

        Node* nd = this->create<Node>(p);
        nd->ref_type = FS_NODE;
        nodes[p] = nd;
        nd->node_type = tt;


        nd->valid_= true;
        return nd;
    }

    // also resolves the reference node:
    Node* mountpointAt(const fs::path& p){
        if(p.begin() == p.end()){
            return nullptr; // empty path is not a mountpoint
        }

        fs::path pm = p;  // will be path to mountpoint description file

        pm.replace_filename( (string_t)p.filename() + MNT_SUFFIX() );

        fs::file_status fstat = symlink_status(pm);

        if(!fs::exists(fstat)) { // if mountpoint description does not exist
            return nullptr; // it is not a mountpoint (not necessary an error)
        }

        // The path represents a mountpoint.
        // From here, if something goes wrong, it is an error
        // and we shall return erroneous node instead of nullptr

        auto it = nodes.find(p);
        if(it != nodes.end()){
            Node* ret = it->second; // node already exists
            if(!ret->is_mountpoint){
                report_error(
                        std::string("Internal Thicket error: existing node is not a mountpoint but must be : ")
                        + p2s(p), SEVERITY_ERROR
                );
                //  ToDo: maybe SEVERITY_PANIC? (now the node type and validity is changed)
                ret->ref_type = REFERENCE_NODE;
                ret->is_mountpoint = true;
                ret->valid_ = false;
                ret->resolved_ = NODE_FAILED_TO_RESOLVE;
            }

            return ret;  // return existing node
        }

        Node* nd = create<Node>(p);
        nd->ref_type = REFERENCE_NODE;
        nd->is_mountpoint = true;
        nodes[p] = nd;

        if(!fs::is_regular_file(fstat)) {
            report_error(
                    std::string("A mountpoint description is not a regular file: ")
                    + p2s(pm), SEVERITY_ERROR
            );

            nd->resolved_ = NODE_FAILED_TO_RESOLVE; // (also valid == false)
            return nd;
        }

        this->readMountpoint(nd, pm);

        return nd;
    }


    Node* nodeAt(const fs::path& p){ // assume p is canonical
        auto it = nodes.find(p);
        if(it != nodes.end()){
            return it->second; // node already exists
        }

        if(
            std::mismatch(p.begin(), p.end(), root_.begin(), root_.end())
            .second !=  root_.end()
        ){
            report_error(std::string("cornus_thicket: nodeAt() ::path out of root ") + p2s(p), SEVERITY_ERROR);
            return nullptr;
        }

        auto parp = p.parent_path();
        if(p.empty() || p.begin() == p.end() || parp.empty() || parp == p){
            report_error(std::string("cornus_thicket: nodeAt() ::Parent path unavailable for ") + p2s(p), SEVERITY_ERROR);
            return nullptr;
        }

        Node* parn = nodeAt(parp);
        if(!parn){
            return nullptr; // no parent
            // do not report error (seems redundant)
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
        }else if(parn->ref_type == REFERENCE_NODE){

            if(parn->resolved_ == NODE_RESOLVING){
                report_error(std::string(
                        "Circular dependency encountered while accessing node at ")
                            + p2s(parn->path_),
                            SEVERITY_ERROR
                );
            }

            // reference node shall be resolved here, so just return null:
            return nullptr;  // may be report error?
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

        if(nd->resolved_ == NODE_RESOLVING){
            report_error(std::string(
                    "Circular dependency encountered while resolving node at ")
                        + p2s(nd->path_),
                        SEVERITY_ERROR
            );
            return nullptr;
        }


        resolve(*nd);
        return nd;
    }

    void resolve(Node& n){ // assuming reference type is set for n
        if(n.resolved_ >= NODE_RESOLVED){
            return;
        }

        if(n.resolved_ == NODE_RESOLVING){
            report_error(std::string(
                    "Circular dependency encountered while resolving a node at ")
                        + p2s(n.path_),
                        SEVERITY_ERROR
            );
            return;
        }


        try{
            switch(n.ref_type){
            case FS_NODE:
                resolveFilesystemNode(n);  // resolve as filesystem object
                return;
            case REFERENCE_NODE:
                resolveReferenceNode(n, true);  // resolve as mountpoint or its descendants
                return;
            default:
                return; // unresolved
            }
        }catch(...){
            report_error(std::string(
                    "Internal Thicket error: exception while resolving a node at ")
                        + p2s(n.path_),
                        SEVERITY_ERROR
            );
        }
    }

    // materialization methods:
    void clean(); // cleans all under scope
    void materializeAsSymlinks(); // materializes all under the scope
    void materializeAsCopy(bool symlinks_inside); // materializes all under the scope

    bool silent_ = false;
    bool force_ = false;

private:
    std::string // error
    processMountRecord(
            Node* nd,  // mountpoint node
            MountRecord& mount_record
    );

    void readMountpoint(
            Node* nd,
            const fs::path& pm // path to mountpoint description file
    );

    Node* resolveMountpointTarget(const fs::path& mountpoint_path, std::string target_path, std::string& errstr);

    void resolveFilesystemNode(Node& n);
    void resolveReferenceNode(Node& n, bool check_resolving);


    void collectRefnodeChildren(Node& n);
    static bool is_thicket_mountpoint_description(const fs::path& p, fs::path* mountpoint_path);

    static void clean(const fs::path& p, std::map<fs::path, bool>& to_delete);
    void materializeAsSymlinks(Node& n);
    void materializeAsCopy(Node& n, bool symlinks_inside);
};

} // namespace

#include "context_read_mountpoint.hpp"
#include "context_resolve_fin.hpp"
#include "context_resolve_ref.hpp"
#include "context_clean.hpp"
#include "context_materialize_symlinks.hpp"
#include "context_materialize_as_copy.hpp"

#endif
