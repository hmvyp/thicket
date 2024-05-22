#ifndef context_read_mountpoint_hpp
#define context_read_mountpoint_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

struct MountRecord {

    std::string target;
    std::string filter; // "**/*" - uiniversal filter, "path/to/file_or_dir" - singular filter
                        // (path/to... is relative to target)
    std::string mount_path; // "." denotes the mountpoint itself


    static constexpr auto&  FILTER_UNIVERSAL = "**.*";

    static std::string parseRecord( // returns error string (emty on success)
            const std::string& src,
            std::vector<MountRecord> push_here
    ){
        size_t part_start_pos = 0;
        std::vector<std::string> parts;

        for(
                size_t pos = src.find_first_not_of(" \t", 0);
                pos != std::string::npos;
                pos = src.find_first_not_of(" \t", part_start_pos)
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

        MountRecord rec;

        if(sz == 1){ // old syntax: target path only
            rec.target = parts[0];
            rec.filter = FILTER_UNIVERSAL;
        }else if ( (sz ==3 || sz == 4) && parts[0] == CORNUS_THICKET_ADD_CLAUSE){
            rec.target = parts[1];
            rec.filter = parts[2];
            if(sz == 4) {
                rec.mount_path = parts[3];
            }
        }else{
            return std::string("Syntax error in mountpoint record: ") + src;
        }

        push_here.push_back(rec);
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
void
Context::readMountpoint(
        Node* nd,
        const fs::path& p, // mountpoint path
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
        if(
                tgn == nullptr
                //|| !tgn->valid_ // (removed; seems redundant since tgn->resolved_ is stronger)
                                  //  (maybe  valid_ makes sense for final nodes only?)
                || tgn->resolved_ != NODE_RESOLVED
        ){
            // nd->resolved_ = NODE_FAILED_TO_RESOLVE; // removed because:
                                                       // 1) it is only partial failure
                                                       // 2) it will be overwritten by resolveReferenceNode()
            report_error( erprfx(eno) +
                        + "can not resolve mountpoint target:\n    "
                        + p2s(ptcn)
                    , SEVERITY_ERROR
            );
            continue;
        }

        nd->targets.push_back(tgn);  // --T v2 todo: sometimes push as "mountpath" target
    }

    // Reference node is useless being unresolved, so resolve it:
    resolveReferenceNode(*nd, false); // NODE_RESOLVING is already set, so pass false as 2nd arg
}

}
#endif
