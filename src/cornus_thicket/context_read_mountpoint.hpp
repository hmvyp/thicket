#ifndef context_read_mountpoint_hpp
#define context_read_mountpoint_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols
#include "filter.hpp"

namespace cornus_thicket {

struct MountRecord {

    std::string target;
    std::string filter; // "**/*" - uiniversal filter, "path/to/file_or_dir" - singular filter
                        // (path/to... is relative to target)

    std::string filter_path;  // path w/o wildcards before pattern
    std::string filter_pattern; // pattern to match the remainder

    Filter filter_;

    std::string mount_path; // "" or  "." denotes the mountpoint itself

    bool optional = false;

    Node* mp_placeholder; // mountpoint placeholder to merge into mountpoint

    MountRecord(Context& ctx, Node* mountpoint_node)
        :mp_placeholder(ctx.createNode(mountpoint_node->get_path()))
    {}

    static constexpr auto&  FILTER_UNIVERSAL = "**/*";

    std::string // returns error string (empty on success)
    parseRecord(
            const VarPool& vpool,
            const std::string& src
    ){
        using std::string;

        // std::cout<< "parsing path " << src;
        size_t part_start_pos = 0;
        std::vector<std::string> parts;

        for(
                size_t pos = src.find_first_of(" \t", 0);
                pos != std::string::npos;
                pos = src.find_first_of(" \t", part_start_pos)
        ){
            if(part_start_pos == pos){
                part_start_pos = pos + 1;
                continue; // skip multiple spaces
            }

            parts.push_back(src.substr(part_start_pos, pos - part_start_pos));
            part_start_pos = pos + 1;
        }

        parts.push_back(src.substr(part_start_pos));

        size_t sz = parts.size();


        if(sz == 1){ // old syntax: target path only
            target = parts[0];
            filter = FILTER_UNIVERSAL;
            //std::cout << "old syntax detected, target: " + rec.target ;
        }else if (
                (sz ==3 || sz == 4)
                && ((parts[0] == CORNUS_THICKET_ADD_CLAUSE) || (optional = (parts[0] == CORNUS_THICKET_ADD_OPTIONAL)))
        ){
            target = parts[1];
            filter = parts[2];
            if(sz == 4) {
                mount_path = parts[3];
            }
        }else{
            return std::string("Syntax error in mountpoint record: ") + src;
        }

        string errstr;

        auto errhandler = [&](std::string errs) -> bool {errstr = errs; return true;}; // return true to stop substitutions

        auto mk_substitutions = [&](std::string& s) -> void {
            s = substituteEscapes(s);
            s = substituteExpressions(
                            vpool,
                            s,
                            errhandler
                    );
        };

        mk_substitutions(target);

        if(!errstr.empty()){
            return errstr;
        }

        mk_substitutions(filter);

        if(!errstr.empty()){
            return errstr;
        }

        mk_substitutions(mount_path);
        if(!errstr.empty()){
            return errstr;
        }

        errstr = parse_filter();


        errstr = errstr.empty()? filter_.setFilter(filter_pattern) : errstr;

        return errstr;
    }

    std::string // error
    parse_filter(){
        using std::string;

        size_t wcard_pos = filter.find("*");

        if(wcard_pos != string::npos){
            size_t last_fpath_slash_pos = filter.rfind("/", wcard_pos);
            size_t filter_path_end_pos = (last_fpath_slash_pos == string::npos)? 0 : last_fpath_slash_pos;

            filter_path = filter.substr(0, filter_path_end_pos);

            size_t filter_patter_pos = (last_fpath_slash_pos == string::npos)
                    ? filter_path_end_pos
                    : filter_path_end_pos + 1; // skip last slash after the filter path

            filter_pattern = filter.substr(filter_patter_pos);

            //if( !(filter_pattern == MountRecord::FILTER_UNIVERSAL)){
            //    return string("Only universal wildcard **/* at the end of a filter is supported ");
            //}

        }else{
            filter_path = filter;
            filter_pattern = MountRecord::FILTER_UNIVERSAL;
        }

        auto slash_pred = [](unsigned char c){
            return c == '/';
        };

        filter_path = trim(filter_path, slash_pred); // hmmm... avoid segmentation fault in "/" fs::path operator...

        return string();
    }
};


inline
Node*
Context::resolveMountpointTarget(
        const fs::path& mountpoint_path,
        std::string target_path,
        std::string& errstr
){
    std::string en = target_path; // copy as en may be changed (leave original untouched)

    bool from_root = (en[0] == '/') ? true : false; // path from the root ("absolute") or relative to mountpoint

    if(from_root){
        en.erase(0,1); // remove "absolute" mark
    }

    auto pas = string2path_string(en); // entry as relative native string
    auto prt = fs::path(pas); // the entry as fs:path

    if(prt.empty()){
        errstr = "results in empty path";
        return nullptr;
    }

    fs::path pt;  // target path

    if(from_root) {
        pt = this->root_ / prt;
    }else{
        pt = mountpoint_path.parent_path() / prt;
    }

    std::error_code err;
    auto ptcn = fs::weakly_canonical(pt, err); // the tail may not exist (e.g. may point to another mountpoint)
    if(err){
        errstr = std::string() +  "mountpoint target\n    "
                + p2s(pt) + "\n    can not be converted to canonical path";
        return nullptr;
    }

    Node* tgn = resolveAt(ptcn); // resolve the target
    if(
            tgn == nullptr
            //|| !tgn->valid_ // (removed; seems redundant since tgn->resolved_ is stronger)
                              //  (maybe  valid_ makes sense for final nodes only?)
            || tgn->resolved_ != NODE_RESOLVED
    ){
        return nullptr; // silently (the target may be optional)
    }

    return tgn;
}


inline
std::string // error
Context::processMountRecord(
        Node* nd,  // mountpoint node
        MountRecord& mrec
){
    using std::string;

    string errstr;

    // Node* nd_push_here = nd; // tmp-2024-08-06
    Node* nd_push_here = mrec.mp_placeholder;

    auto createDescendants = [&](const string& path_as_string) -> void {
        // create descendants with (has_own_content_ == true) and w/o targets
        // then point nd_push_here to the last one
        for(size_t pos = 0; pos != string::npos; ){
            size_t pos_slash = path_as_string.find("/", pos);
            string pathelem = path_as_string.substr(pos, pos_slash - pos); // pos_slash may be npos

            if(!pathelem.empty()){
                Node* child = ensureChild(nd_push_here, string2path_string(pathelem));

                // --T 2024-06-14  do not try to calculate node_type here!
                // (do not bypass detectRefnodeType()) which does some side work)

                nd_push_here->has_own_content_ = true; // --T 2024-07-02 the parent has own content
                                              // (it has at least an explicit child), the child may or may not have its own content
                child->ref_type = REFERENCE_NODE;
                child->valid_ = true;

                nd_push_here = child;
            }

            if(pos_slash == string::npos) {
                break;
            }else{
                pos = pos_slash + 1;
            }
        }
    };


    const std::string&
    full_target_pathstring = (mrec.filter_path.empty())
            ? mrec.target
            : (mrec.target + "/" + mrec.filter_path);

    Node* tgn_to_push =  resolveMountpointTarget(nd->get_path() , full_target_pathstring, errstr);

    if(tgn_to_push == nullptr){
        if(!errstr.empty()){
            return string("Error while resolving mountpoint target at ") + full_target_pathstring + " The error: " + errstr;
        }else if (!mrec.optional){
            return string("Can not resolve mandatory mountpoint target ") + full_target_pathstring;
        }else{
            return string(); // silently return in case of optional record
        }
    }

    // mountpoint target is Ok here.

    // create descendants along mountpath:
    if(!mrec.mount_path.empty()) {
        createDescendants(mrec.mount_path); // moves nd_push_here to the farthest descendant
    }

    // create descendants along filter_path
    // from the nd_push_here and point nd_push_here to the last one:
    createDescendants(mrec.filter_path); // moves nd_push_here to the farthest descendant

    apply_filter(
            *tgn_to_push,
            tgn_to_push->path_as_string_.length(),//size_t start_filtering_from, // (in the target path string)
            fs::path(),
            nd_push_here, // optional may be null (if needed, it is created inside)
            mrec.filter_
    );

    resolveReferenceNode(*mrec.mp_placeholder, true);

    errstr = mergeNodes(nd, mrec.mp_placeholder);

    return errstr;
}


inline
std::string // error
Context::mergeNodes(
        Node* nd,  // merge into
        Node* from  // assuming resolved
){

    nd->valid_ = nd->valid_ || from->valid_; // ??? at least one is valid?

    if(nd->node_type == UNKNOWN_NODE_TYPE){
        nd->node_type = from->node_type;
    }


    // is_mountpoint  - do not touch


    if(nd->node_type != from->node_type) {
        return std::string("can not merge nodes of different types (regular file vs. directory)");
    }

    nd->ref_type = REFERENCE_NODE;


    if(from->has_own_content_){
        nd->has_own_content_ = true;
    }

    for(auto& trg : from->targets){
        nd->targets.push_back(trg);
    }

    // merge final targets... let's leave it to resolveReferenceNode()

    for(auto& chre: from->children){
        Node* ch = ensureChild(nd, chre.first);
        mergeNodes(ch, chre.second);
    }

    nd->children_collected = true; // prevent resolveReferenceNode() from redundant collection of children and targets
    //(that does not prevent subsequent calls of mergeNodes() from addition of another children)

    return std::string();
}



inline
void
Context::readMountpoint(
        Node* nd,
        const fs::path& pm // description file path
){
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
        return;
    }


    nd->valid_ = true; // (from here only partial fails are possible)


    // common error prefix lambda:
    auto erprfx = [&pm](const std::string& mnt_entry) -> std::string {
        return
             std::string("\n    in mountpoint description file:\n    ") + p2s(pm)
             + "\n    mountpoint entry:\n    " + mnt_entry
             + "\n    ";
    };

    nd->resolved_ = NODE_RESOLVING; // to catch circular dependencies

    // run over mountpoint entries to calculate and resolve targets:

    for(auto& eno : mt){
        MountRecord mount_record(*this, nd);

        std::string errstr = mount_record.parseRecord(this->varpool, eno);

        if(!errstr.empty()){
            report_error(erprfx(eno) + errstr, SEVERITY_ERROR);
            continue;
        }

        if(!(errstr = processMountRecord(nd, mount_record)).empty()) {
            report_error(erprfx(eno) + errstr, SEVERITY_ERROR);
            continue;
        }
    }

    // Reference node is useless being unresolved, so resolve it:
    resolveReferenceNode(*nd, false); // NODE_RESOLVING is already set, so pass false as 2nd arg

    publishTree(*nd);
}

}
#endif
