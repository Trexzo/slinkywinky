// R252 strictly parses the actual disposable R251 Java8 lease receipt.
// This is a test-only cross-language ABI check; stdout is not an OS
// authentication source and never grants a game/native owner.
#include "slinkywinky/jvm_witnessed_native_recovery.hpp"
#include <iostream>
#include <string>
int main(int argc,char** argv){
    if(argc!=6)return 2;
    const std::string expectedLine=std::string("R252_LEASE_RECEIPT ")+
        argv[1]+" "+argv[2]+" "+argv[3]+" "+argv[4];
    const auto expected=slinkywinky::parse_r252_fixture_receipt(expectedLine);
    const auto observed=slinkywinky::parse_r252_fixture_receipt(argv[5]);
    if(!expected || !observed || !(*expected==*observed))return 3;
    std::cout<<"R252_JAVA8_ACTUAL_R251_RECEIPT_NATIVE_ABI_PASS\n";
    return 0;
}
