#ifndef context_read_mountpoint_hpp
#define context_read_mountpoint_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

struct MountRecord {

    std::string target;
    std::string filter; // "**/*" - uiniversal filter, "path/to/file_or_dir" - singular filter
                        // (path/to... is relative to target)
    std::string mount_path; // "" or  "." denotes the mountpoint itself

    bool optional = false;


    static constexpr auto&  FILTER_UNIVERSAL = "**/*";

    std::string parseRecord( // returns error string (emty on success)
            const std::string& src
    ){

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

            //std::cout << "\n pos: " << pos << " part_start pos: "  << part_start_pos;
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

        return std::string();
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

    // std::cout<< "target path passed: " + target_path;

    bool from_root = (en[0] == '/') ? true : false; // path from the root ("absolute") or relative to mountpoint

    if(from_root){
        en.erase(0,1); // remove "absolute" mark
    }

    auto pas = string2path_string(en); // entry as relative native string
    auto prt = fs::path(pas); // the entry as fs:path

    if(prt.empty()){
        // nd->resolved_ = NODE_FAILED_TO_RESOLVE;
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
        // nd->resolved_ = NODE_FAILED_TO_RESOLVE;
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
        // nd->resolved_ = NODE_FAILED_TO_RESOLVE; // removed because:
                                                   // 1) it is only partial failure
                                                   // 2) it will be overwritten by resolveReferenceNode()
        errstr =
                    + "can not resolve mountpoint target:\n    "
                    + p2s(ptcn);
        return nullptr;
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
    Node* tgn = resolveMountpointTarget(nd->get_path() , mrec.target, errstr);
    if(tgn == nullptr || !errstr.empty()){
        return errstr;
    }

    // analyze the filter:

    // bool has_wcard = false;

    size_t wcard_pos = mrec.filter.find("*");

    string filter_path = mrec.filter.substr(0, wcard_pos); // wcard_pos == string::npos is also  ok

    auto slash_pred = [](unsigned char c){
        return c == '/';
    };

    filter_path = trim(filter_path, slash_pred); // hmmm... avoid segmentation fault in "/" fs::path operator...

    if(wcard_pos != string::npos){
        string wcard = mrec.filter.substr(wcard_pos);
        if(wcard == MountRecord::FILTER_UNIVERSAL){
            // has_wcard = true;
        }else{
            return string("Only universal wildcard */** at the end of a filter is supported ");
        }
    }

    Node* tgn_to_push = tgn;  // A node the filter path points to (inside the whole target node)
    Node* nd_push_here = nd;

    auto createDescendants = [&](const string& path_as_string) -> void {
        // create descendants with (has_own_content_ == true) and w/o targets
        // then point nd_push_here to the last one
        for(size_t pos = 0; pos != string::npos; ){
            size_t pos_slash = path_as_string.find("/", pos);
            string pathelem = path_as_string.substr(pos, pos_slash); // pos_slash may be npos

            if(!pathelem.empty()){
                Node* child = ensureChild(nd_push_here, string2path_string(pathelem));
                child->has_own_content_ = true; // stop following  references
                child->ref_type = REFERENCE_NODE;
                child->node_type = DIR_NODE; // may be overridden
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


    if(!mrec.mount_path.empty()) {
        // create mountpath as Nodes graph, i.e.
        createDescendants(mrec.mount_path); // moves nd_push_here to the farthest descendant
    }

    if(!filter_path.empty()) {
        // std::cout << "\n filter path: " << filter_path;
        tgn_to_push = resolveAt(tgn->get_path() / fs::path(string2path_string(filter_path)));
        // std::cout << "\n filter path (after): " << filter_path;
        if(tgn_to_push == nullptr){
            return string("Can not resolve filter path ") + filter_path + " inside mountpoint target" ;
        }

        // create descendants with (has_own_content_ == true) along filter_path
        // from the nd_push_here and point nd_push_here to the last one:

        createDescendants(filter_path); // moves nd_push_here to the farthest descendant
    }

    nd_push_here->node_type = tgn_to_push->node_type;
    nd_push_here->targets.push_back(tgn_to_push);

    return errstr;
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
             + "\n    mountpoint target:\n    " + mnt_entry
             + "\n    ";
    };

    nd->resolved_ = NODE_RESOLVING; // to catch circular dependencies

    // run over mountpoint entries to calculate and resolve targets:
    for(auto& eno : mt){

        MountRecord mount_record;
        auto errstr = mount_record.parseRecord(eno);

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
}

}
#endif
