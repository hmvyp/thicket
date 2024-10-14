#ifndef cornus_thicket_imprint_wrapper_hpp
#define cornus_thicket_imprint_wrapper_hpp

#include "imprint.hpp"

namespace cornus_thicket {


class ImprintWrapper{

public:
    ImprintWrapper() = default;
    Imprint* getImprint(){return pimp_;}

private:
    ImprintWrapper(ImprintWrapper&&) = delete; // non-copiable

    Imprint* pimp_ = nullptr;
    friend class ImprintControl;
};

// RAII accessor to ImprintWrapper
class ImprintControl{
public:
    ImprintControl(ImprintWrapper& iw)
    : iw_(iw){

    }

    ~ImprintControl(){
        if(new_imp_){
            new_imp_->writeImprintFile();
            delete new_imp_;
            iw_.pimp_ = old_imp_;
        }
    }

    void newImprint(fs::path imp_file){
        if(new_imp_) return; // already created

        new_imp_ = new Imprint();
        new_imp_->setImprintFilePath(imp_file);

        old_imp_ = iw_.pimp_;
        iw_.pimp_ = new_imp_;
    }

    Imprint* getImprint(){
        return iw_.getImprint();
    }

private:
    ImprintControl(ImprintControl&&) = delete; // non-copiable

    ImprintWrapper& iw_;

    Imprint* new_imp_ = nullptr;
    Imprint* old_imp_ = nullptr;

};

} // namespace
#endif
