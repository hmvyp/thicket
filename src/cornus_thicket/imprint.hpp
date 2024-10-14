#ifndef cornus_thicket_imprint_hpp
#define cornus_thicket_imprint_hpp

#include <cornus_thicket/special_files.hpp>
#include <string_view>
#include <fstream>
#include <iostream>

#include  "utils.hpp"
#include  "node.hpp"


namespace cornus_thicket {


enum NodeStatus {
    nsNONE,
    nsEXISING, // not a thicket artifact (pre-existed)
    nsLINK, // symbolic link (an artifact)
    nsCOPY //  an artifact, but not a link, i.e. actual file/directory created by thicket
};

inline const char* verboseNodeStatus(NodeStatus ns){
    switch(ns){
    case nsNONE: return "UNKNOWN_NODE_STATUS";
    case nsEXISING: return "Pre-existed Object";
    case nsLINK: return "Link Artifact";
    case nsCOPY: return "Copy Artifact";
    default: return " Node status out of range";
    }
}

class Imprint {
public:

    void reset(){
        all_records_.clear();
        garbage_.clear();
    }

    void setImprintFilePath(const fs::path& imp_path){
        imprint_file_ = imp_path;
        scope_ = imp_path.parent_path();
        scope_as_string_ = p2s(scope_);
    }

    static
    unsigned // error count
    collectImprintsInside(const fs::path& cur_dir, std::list<Imprint>& imprints){
        unsigned errcount = 0;

        fs::path dot_imprint = cur_dir/imprint_suffix;
        if(fs::exists(dot_imprint)) {
            Imprint imp;
            switch(imp.readImprint(dot_imprint)){
            case Imprint_READ_NOFILE:
                report_error(std::string(" Inprint file read i/o error") + p2s(dot_imprint), SEVERITY_ERROR);
                [[fallthrough]];
            case Imprint_READ_ERROR: // (already reported inside readImprint() )
                ++errcount;
                break;
            case Imprint_READ_OK: // go further
                imp.setImprintFilePath(dot_imprint);
                imprints.push_back(std::move(imp));
            }
        }

        std::set<fs::path> mountpoints_inside_cur_dir;
        std::list<fs::path> subdirs;

        for (auto const& de : fs::directory_iterator{cur_dir}){
             const auto& p = de.path();

             fs::path mountpoint_path;
             if(is_thicket_imprint(p, &mountpoint_path)) {

                 mountpoints_inside_cur_dir.insert(std::move(mountpoint_path));  // moved, do not use further!!!

                 Imprint imp;

                 switch(imp.readImprint(p)){
                 case Imprint_READ_NOFILE:
                     report_error(std::string(" Inprint file read i/o error") + p2s(p), SEVERITY_ERROR);
                     [[fallthrough]];
                 case Imprint_READ_ERROR: // (already reported inside readImprint() )
                     ++errcount;
                     continue;
                 case Imprint_READ_OK: // go further
                     ;
                 }

                 imp.setImprintFilePath(p);
                 imprints.push_back(std::move(imp));
                 continue;
             }

             if(fs::is_directory(p)){
                 subdirs.push_back(p);
             }
        }

        for(const auto& subdir: subdirs){
            if(mountpoints_inside_cur_dir.find(subdir) != mountpoints_inside_cur_dir.end()){
                continue; // do not recurse into mountpoints
            }

            errcount += collectImprintsInside(subdir, imprints);
        }


        return errcount;
    }

    unsigned collectAndDeleteArtifacts(){
        unsigned errcount = 0;

        errcount += collectAllArtifacts();

        if(errcount) return errcount;

        return deleteGarbage();
    }

    unsigned // error count
    collectAllArtifacts()
    {
        unsigned errcount = 0;

        errcount = collectArtifactsInExistingDir(scope_);
        all_records_.clear(); // we do not need them more

        if(errcount != 0){
            std::cout << " \n Errors while collecting artifacts (see previous messages). Nothing is deleted \n";
            return errcount;
        }

        // artifact collection Ok.

        return errcount;
    }

    unsigned // error count
    deleteGarbage()
    {
        unsigned errcount = 0;
        for(auto& p : garbage_){
            std::error_code er;
            remove_all(p, er);
            if(er){
                ++errcount;
                std::cout << " \n Error while deleting " << p2s(p);
                report_error( std::string("Error while deleting artifact file:  " )
                        + p2s(p)
                        , SEVERITY_ERROR // maybe PANIC?
                );
            }
        }

        garbage_.clear();


        if(errcount) {
            std::cout << " \n Errors while deleting artifacts (see previous messages). Something is NOT deleted";
            return errcount;
        }



        // Artifacts deleted. Try to delete imprint file:
        std::error_code erc;

        fs::remove(imprint_file_, erc);

        if(erc){
            ++errcount;
            report_error( std::string("Error while deleting thicket imprint (artifacts description):  ")
                    + p2s(imprint_file_)
                    , SEVERITY_ERROR
            );

            return errcount;
        }

        return errcount; // i.e. 0
    }


    std::string // error or ""
    addArtifact(Node& n, NodeStatus ns){
        std::string ret_err;

        const std::string_view npsv = std::string_view(n.path_as_string_);
        const std::string_view sp_sv = std::string_view(scope_as_string_);


        if(n.node_type == UNKNOWN_NODE_TYPE){
            ret_err = "Internal Thicket error: "" Can not add artifact  node type (file or directory) is unknown";
        }else if(n.ref_type == FS_NODE && ns > nsEXISING) {
            ret_err = "Internal Thicket error:  Can not add artifact  pre-existed node can not be a link or copy";
        }else if(n.ref_type == REFERENCE_NODE && ns == nsEXISING) {
            ret_err = "Internal Thicket error:  Can not add artifact: virtual (reference) node can not have nsEXISING status";
        }else if( // check if the node path is really prefixed with the scope path (paranoia):
                npsv.length() < sp_sv.length() || npsv.substr(0, sp_sv.length()) != sp_sv
        ){
            ret_err = "Internal Thicket error:  for a node under the scope the node path is not prefixed with the scope path ";
        }

        if(!ret_err.empty()){
            report_error(
                    ret_err
                    + " Node at: "
                    + n.path_as_string_
                    , SEVERITY_ERROR
            );

            return ret_err;
        }

        Record rc = {n.node_type, ns};

        std::string key = (npsv.length() == sp_sv.length())
                ? ""
                : std::string(npsv.substr(sp_sv.length() + 1)); // remove slash before relative path

        all_records_[key] = rc;

        return "";
    }

    unsigned // error count
    writeImprintFile(){
        unsigned errcount = 0;


        try{
            std::ofstream os(imprint_file_, std::ios::binary);
            os << IMPRINT_SIGNATURE << '\n';
            os << "# This is a generated file containing descriptions of Thicket artifacts created in this scope. Do not edit." << '\n';
            for(auto it = all_records_.begin(); it != all_records_.end(); it++){
                std::string s;
                auto errstr = encodeRecord(it->first, it->second, s);
                if(!errstr.empty()){
                    ++errcount;
                    report_error( std::string("Error while creating imprint file: \n ")
                            + errstr
                            + "\n for artifact: "
                            +it->first
                            , SEVERITY_ERROR
                    );
                    continue;
                }

                os << s << '\n';
            }
        }catch(...){
            ++errcount;
            report_error( std::string("I/O error while creating imprint file")
                    , SEVERITY_ERROR
            );
        }

        return errcount;
    }


private:

    static inline const char* IMPRINT_SIGNATURE = "thicket imprint file version 1.0";

    // artifact description record:
    struct Record {
        NodeType ntype; // file or dir
        NodeStatus nstatus;
    };

    enum ImprintFileReadResult{
      Imprint_READ_OK = 0,
      Imprint_READ_NOFILE,
      Imprint_READ_ERROR
    };

    ImprintFileReadResult
    readImprint(const fs::path& imprint_path){
        std::error_code err_exists;

        if(!fs::exists(imprint_path, err_exists)){
            if(err_exists){
                report_error( std::string("Error while checking imprint file existence. File:  " )
                        + p2s(imprint_path)
                        , SEVERITY_ERROR
                );
                return Imprint_READ_ERROR;
            }else{
                return Imprint_READ_NOFILE;
            }
        }

        try{ // read imprint file:
            std::ifstream is(imprint_path);
            std::stringstream buffer;
            buffer << is.rdbuf();

            unsigned line_no = 0;
            std::string line;

            while(std::getline(buffer, line)) {
                ++line_no;

                if(line_no == 1){
                    if(line != IMPRINT_SIGNATURE) {
                        report_error( std::string("Invalid imprint file version. File:  " )
                                + p2s(imprint_path)
                                + " At line "
                                + std::to_string(line_no)
                                , SEVERITY_ERROR
                        );
                        return Imprint_READ_ERROR;
                    }

                    continue;
                }

                if(line.empty() || line[0] == '#'){ // empty or comment
                    continue;
                }

                auto errstr = decodeRecord(line);
                if(!errstr.empty()){
                    report_error( std::string("Imprint file corrupted at line No " ) + std::to_string(line_no)
                            , SEVERITY_ERROR
                    );
                    return Imprint_READ_ERROR;
                }
            }
        }catch(...){
            report_error( std::string("I/O error while reading imprint file ") + p2s(imprint_path), SEVERITY_ERROR);
            return Imprint_READ_ERROR;
        }

        setImprintFilePath(imprint_path);


        return Imprint_READ_OK;
    }

    std::string //error
    encodeRecord(const std::string& relpathstr, Record& rc, std::string& output){
        switch(rc.nstatus){
        case nsEXISING:
            output += 'e';
            break;
        case nsLINK:
            output += 'l';
            break;
        case nsCOPY:
            output += 'c';
            break;
        default:
            return "Internal Thicket error: Unknown artifact node status (existing? link? copy?) ";
        }

        switch(rc.ntype){
        case FILE_NODE:
            output +=  'f';
            break;
        case DIR_NODE:
            output +=  'd';
            break;
        default:
            return "Internal Thicket error: Unknown artifact node type (file? directory?) ";
        }

        output += ':';
        output += relpathstr;

        return "";
    }

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


    Record*
    findRecord(
            const fs::path& p, // absolute path
            std::string& relpath // output (p relative to scope)
    ){
        relpath = p2s(p.lexically_proximate(scope_));

        auto rec_it = all_records_.find(relpath);

        if(rec_it == all_records_.end()){
            return nullptr; // do nothing with unknown objects and do not recurse into them
        }

        return &rec_it->second;
    }


    unsigned // error count
    collectArtifactsInExistingDir(const fs::path& dirp)
    {
        unsigned errcount = 0;

        for (auto const& de : fs::directory_iterator{dirp}){
            auto& p = de.path();

            if(p.empty()){
                continue; // seems impossible
            }

            std::string relpath;
            const Record* prec = findRecord(p, relpath);

            if(prec == nullptr || prec->nstatus == nsEXISING ){ //existing or unknown
                if(fs::is_directory(p) && !fs::is_symlink(p)){
                    errcount += collectArtifactsInExistingDir(p); // clean child directory
                } // (else do nothing with existing or unknown object)
            }else{ // reference (artifact) case
                errcount += collectArtifact(
                        p,
                        relpath,
                        prec
                );
            }
        }

        return errcount;
    }


    unsigned // error count
    collectArtifactsInReferenceDir(const fs::path& dirp)
    {
        unsigned errcount = 0;

        for (auto const& de : fs::directory_iterator{dirp}){
            auto& p = de.path();
            std::string relpath;
            const Record* prec = findRecord(p, relpath);
            if(prec == nullptr){
                ++errcount;
                report_error(
                    std::string("Unexpected object inside an artifact directory: ") + relpath
                        +
                        "\n    Please check it, move it to an appropriate place or delete it manually"
                    ,SEVERITY_ERROR
                );
            }

            errcount += collectArtifact(
                    p,
                    relpath,
                    prec
            );

            if(errcount == 0){
                garbage_.push_back(p);
            }
        }

        return errcount;
    }


    unsigned // error count
    collectArtifact(
            const fs::path& p,
            const std::string& relpath,
            const Record* prec // assuming non-null
    ){
        unsigned errcount = 0;

        const NodeStatus nstatus = fs::is_symlink(p) ? nsLINK : nsCOPY;

        // compare file and record status and type, if they differ, report error and leave the object untouched
        if(nstatus != prec->nstatus){
            report_error(
                std::string("For artifact located at: ") + relpath
                    + "\n    artifact status mismatch.\n Previously created as "
                    + verboseNodeStatus(prec->nstatus)
                    + "\n    but now found as "
                    + verboseNodeStatus(nstatus)
                    + "\n    Possible error cause: "
                      "\n    last Thicket invocation has been performed under different OS."
                      "\n    (this is especially likely for virtulization environment)"
                      "\n    In this case return to another OS to clean up Thicket artifacts"
                ,SEVERITY_PANIC  // very dangerous case
            );

            ++errcount;
        }else if(nstatus == nsLINK || fs::is_regular_file(p)){
            garbage_.push_back(p);
        }else if(fs::is_directory(p)){
            const unsigned ec = collectArtifactsInReferenceDir(p);
            if(ec == 0){
                garbage_.push_back(p); // collect the directory itself
            }

            errcount += ec;
        }
        return errcount;
    }

    fs::path imprint_file_;

    fs::path scope_;

    std::string scope_as_string_;

    std::map<std::string, Record> all_records_;

    std::list<fs::path> garbage_;

}; // class Imprint

} // namespace

#endif
