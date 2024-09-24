#ifndef cornus_thicket_imprint_hpp
#define cornus_thicket_imprint_hpp

#include  "node.hpp"


namespace cornus_thicket {

class Imprint {

    // Artifact type:
    enum NodeStatus {
        nsNONE,
        nsEXISING, // not a thicket artifact (pre-existed here)
        nsLINK, // symbolic link
        nsCOPY //  an artifact, but not a link, i.e. actual file/directory created by thicket
    };

    struct Record {
        NodeType ntype; // file or dir
        NodeStatus nstatus;
    };

private:
    std::string // error
    parseRecord(const std::string& line){
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

        all_nodes_[line.substr(3)] = rc;

        return "";
    }


    std::map<std::string, Record> all_nodes_;

};




}

#endif
