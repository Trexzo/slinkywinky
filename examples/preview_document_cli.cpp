#include "slinkywinky/menu_preview_document.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string_view>
using namespace slinkywinky;
namespace fs=std::filesystem;
namespace {
void usage() {
    std::cerr<<"Offline preview document tool (not a game configuration)\n"
             <<"  slinkywinky_preview_document create <output.swp>\n"
             <<"  slinkywinky_preview_document inspect <input.swp>\n"
             <<"  slinkywinky_preview_document copy <input.swp> <output.swp>\n";
}
bool available_output(const fs::path& path) {
    if(path.empty())return false;
    std::error_code error;
    const auto exists=fs::exists(path,error);
    return !error&&!exists;
}
int run(int argc,char** argv) {
    if(argc<3){usage();return 2;}
    const std::string_view command=argv[1];
    if(command=="create" && argc==3) {
        const fs::path output(argv[2]);
        if(!available_output(output) ||
           !save_menu_preview_document(output,MenuPreviewDocument{})) {
            std::cerr<<"Could not create preview document; output must not already exist.\n";
            return 1;
        }
        std::cout<<"Created default offline preview document.\n";return 0;
    }
    if((command=="inspect" && argc==3)||(command=="copy" && argc==4)) {
        const auto doc=load_menu_preview_document(fs::path(argv[2]));
        if(!doc){std::cerr<<"Invalid or incompatible preview document.\n";return 1;}
        if(command=="copy") {
            const fs::path output(argv[3]);
            if(!available_output(output)||!save_menu_preview_document(output,*doc)) {
                std::cerr<<"Could not copy preview document; output must not already exist.\n";
                return 1;
            }
            std::cout<<"Copied canonical offline preview document.\n";return 0;
        }
        // Summary intentionally excludes profile names and user-entered text.
        std::cout<<"format=1\ncatalog_fingerprint="<<std::hex
                 <<std::setw(16)<<std::setfill('0')<<menu_preview_catalog_fingerprint()
                 <<std::dec<<"\nprofiles="<<doc->profiles.size()
                 <<"\nwidget_overrides="<<doc->values.size()
                 <<"\ncompact="<<doc->compact<<"\nwide="<<doc->wide
                 <<"\nruntime_settings=excluded\n";
        return 0;
    }
    usage();return 2;
}
}
int main(int argc,char** argv) {
    try{return run(argc,argv);}
    catch(const std::exception& e){std::cerr<<"Preview document operation failed: "<<e.what()<<'\n';return 1;}
}
