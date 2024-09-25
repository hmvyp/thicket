#ifndef cornus_thicket_imprint_hpp
#define cornus_thicket_imprint_hpp

#include <string_view>
#include  "node.hpp"


namespace cornus_thicket {

class Imprint {

    // Artifact type:
    enum NodeStatus {
        nsNONE,
        nsEXISING, // not a thicket artifact (pre-existed)
        nsLINK, // symbolic link (an artifact)
        nsCOPY //  an artifact, but not a link, i.e. actual file/directory created by thicket
    };

    struct Record {
        NodeType ntype; // file or dir
        NodeStatus nstatus;
    };

private:
    std::string // error
    decodeRecord(const std::string& line){

        // expected record structure example: cd:relative/path/from/scope
        // 'c' - "copy"
        // 'd' - "directory"
        // ':' - delimiter

        size_t len = line.length();
        if(len == 0 || line[0] == '#'){
            return ""; // skip empty or comments
        }

        if(line.length() < 3){
            return "Imprint record too short";
        }

        Record rc = {UNKNOWN_NODE_TYPE, nsNONE};

        switch(line[0]){
        case 'e':
            rc.nstatus = nsEXISING;
            break;
        case 'l':
            rc.nstatus = nsLINK;
            break;
        case 'c':
            rc.nstatus = nsCOPY;
            break;
        default:
            return "Imprint record syntax: 1st symbol shall be 'e', 'l' or 'c' ";
        }

        switch(line[1]){
        case 'f':
            rc.ntype = FILE_NODE;
            break;
        case 'd':
            rc.ntype = DIR_NODE;
            break;
        default:
            return "Imprint record syntax: 2nd symbol shall be 'f' or 'd' ";
        }


        if(line[2] != ':'){
            return "Imprint record syntax: 3d symbol shall be ':' ";
        }

        all_records_[line.substr(3)] = rc;

        return "";
    }


    std::string // error or ""
    addRecord(const std::string& scope_path, Node& n, NodeStatus ns){
        if(n.node_type == UNKNOWN_NODE_TYPE){
            return "Internal Thicket error: "" Can not encode imprint record: node type (file or directory) is unknown";
        }

        if(n.ref_type == FS_NODE && ns > nsEXISING) {
            return "Internal Thicket error:  Can not encode imprint record: pre-existed node can not be a link or copy";
        }

        if(n.ref_type == REFERENCE_NODE && ns == nsEXISING) {
            return "Internal Thicket error:  Can not encode imprint record: virtual (reference) node can not have nsEXISING status";
        }

        const std::string_view npsv = std::string_view(n.path_as_string_);
        const std::string_view sp_sv = std::string_view(scope_path);

        // check if the node path is really prefixed with the scope path (paranoia):
        if(npsv.length() < sp_sv.length() || npsv.substr(0, sp_sv.length()) != sp_sv) {
            return "Internal Thicket error:  for a node under the scope the node path is not prefixed with the scope path ";
        }

        Record rc = {n.node_type, ns};

        all_records_[std::string(npsv.substr(sp_sv.length()))] = rc;

        return "";
    }


    std::map<std::string, Record> all_records_;

};




}

#endif
