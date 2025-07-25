#ifndef cornus_thicket_context_hpp
#define cornus_thicket_context_hpp

#include <cornus_thicket/special_files.hpp>
#include "utils.hpp"
#include "node.hpp"
#include "escapes.hpp"
#include "expressions.hpp"
#include "filter.hpp"
#include "imprint_wrapper.hpp"
#include <streambuf>
#include <fstream>
#include <cctype>
#include <unordered_map>


namespace cornus_thicket {


struct MountRecord; //forward

class Context
        : public ObjectFactory
{
    fs::path root_;  // converted to canonical
    fs::path scope_; // converted to canonical

    std::unordered_map<fs::path, Node*,  FsHashFunc> nodes{50000};

    VarPool varpool;

    ImprintWrapper imprint_wrap_; // holds current imprint while materializing

    Context(Context&&) = delete; // non-copiable

public:

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

        setScope(root_/scope);

        mkRootAndScopeNodes();
    }

    Context(
            unsigned root_level, // number of levels above the scope
            fs::path scope  // in this case: absolute or relative to the current directory
    ){
        setScope(scope);

        fs::path root = scope_;

        for(unsigned i =  root_level; i != 0; i--){
            root = root.parent_path();
        }

        root_ = root;

        mkRootAndScopeNodes();
    }

    const fs::path& getScope(){return scope_;};

    VarPool& getVarPool(){return this->varpool;}

    Node* createNode(const fs::path&  p){
        return create<Node>(p);
    }


private:

    void setScope(const fs::path& scope){
        std::error_code err;
        scope_ =  fs::canonical(scope, err);

        if(err){
            report_error(
                    std::string("cornus_thicket::Context: Invalid scope path") + p2s(scope),
                    SEVERITY_PANIC
            );
        }
    }

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

    Node* existingFileAt(
            const fs::path& p,
            fs::file_status* psymlink_status = nullptr // symlink_status() is costly, so pass it if known
    ){
        auto it = nodes.find(p);
        if(it != nodes.end()){
            Node* ret = it->second; // node already exists
            return ret; // maybe check if node is final and report an internal error otherwise?
        }


        std::error_code ec;

        fs::file_status fstat = (psymlink_status == nullptr)?
                fs::symlink_status(p, ec) :
                *psymlink_status;

        NodeType tt = UNKNOWN_NODE_TYPE;

        if(fs::is_symlink(fstat) || fs::is_regular_file(fstat)){
            tt = FILE_NODE;
        }else if(fs::is_directory(fstat)){
            tt = DIR_NODE;
        }else{
            return nullptr;  // silently skip exotic filesystem objects
        }

        // from here it seems that it is final node
        // (we hope the caller previously checks if it is a mountpoint description or thicket artifact)

        Node* nd = createNode(p);
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

        pm.replace_filename( (string_t)p.filename() + mountpoint_suffix);

        fs::file_status fstat = symlink_status(pm);

        if(!fs::exists(fstat)) { // if mountpoint description does not exist
            return nullptr; // it is not a mountpoint (not necessary an error)
        }

        // The path represents a mountpoint.
        // From here, if something goes wrong, it is an error

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

        Node* nd = createNode(p);
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
            Node* n = it->second;
            if(n->ref_type == REFERENCE_NODE  &&  n->resolved_ == NODE_RESOLVING){
                report_error(std::string(
                        "Circular dependency encountered while accessing node at ")
                            + p2s(n->path_),
                            SEVERITY_ERROR
                );
                return nullptr;
            }

            return n;
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

    // cleaning methods:

    void clean_using_mounts(); // cleans all under the scope using mountpoint description files

    unsigned // errors count
    clean_using_imprint(){  // cleans all under the scope using imprint from previous invocation
        unsigned errcount = 0;

        ImprintProps improps = {this->force_};

        // return imprint.deleteArtifacts();

        std::list<Imprint> imprints;
        errcount += Imprint::collectImprintsInside(this->scope_, improps, imprints);

        if(errcount) return errcount;

        for(auto& imp : imprints){
            unsigned ec = imp.collectAndDeleteArtifacts();
            if(ec) return ec;
        }

        return errcount; // ==0
    }

    // materialization methods:
    void materializeAsSymlinks(); // materializes all under the scope
    void materializeAsCopy(bool symlinks_inside); // materializes all under the scope

    bool silent_ = false;
    bool force_ = false;

private:

    Node* ensureChild(Node* parent, string_t chname){
        auto my_child_entry = parent->children.find(chname);
        Node* my_child = nullptr;

        if (my_child_entry != parent->children.end()){ // child already exists?
            my_child = my_child_entry->second;
        }else{
            my_child = createNode(parent->path_/chname);
            parent->children[chname] = my_child;
        }

        return my_child;
    }

    // publishTree() adds all tree nodes to nodes hashtable.
    void publishTree(Node& n){
        auto it = nodes.find(n.path_);
        if(it == nodes.end()){
            nodes[n.path_] = &n; // publish node
        }

        for(auto& ch_entry : n.children){
            publishTree(*ch_entry.second);
        }
    }

    std::string // error
    processMountRecord(
            Node* nd,  // mountpoint node
            MountRecord& mount_record
    );

    std::string // error
    mergeNodes(
            Node* nd,  // mountpoint node
            Node* from
    );

    Node*
    apply_filter(
            Node& cur_target_node,
            size_t start_filtering_from, // (in the target path string)
            const fs::path& parent_path,
            Node* cur_node, // optional may be null (if needed, it is created inside)
            Filter& flt
    );

    void readMountpoint(
            Node* nd,
            const fs::path& pm // path to mountpoint description file
    );

    Node* resolveMountpointTarget(const fs::path& mountpoint_path, std::string target_path, std::string& errstr);

    void resolveFilesystemNode(Node& n);
    void resolveReferenceNode(Node& n, bool check_resolving);


    void collectRefnodeChildren(Node& n);

    static void clean_using_mounts(const fs::path& p, std::map<fs::path, bool>& to_delete);

    void mk_symlink(Node& n, const Node& to);
    void materializeAsSymlinks(Node& n);
    void materializeAsCopy(Node& n, bool symlinks_inside);
};

} // namespace

#include "context_read_mountpoint.hpp"
#include "context_resolve_fin.hpp"
#include "context_resolve_ref.hpp"
#include "filter_apply.hpp"
#include "context_clean.hpp"
#include "context_materialize_symlinks.hpp"
#include "context_materialize_as_copy.hpp"

#endif
