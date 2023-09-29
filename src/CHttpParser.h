#pragma  once
#include <map>
#include <string>
#include "Utils.h"
using namespace std;

struct HttpMetadata {
    std::string url;
    std::string method;
    std::string protocol;
    std::string body;
    map<std::string, std::string> headers;
};
class CHttpParser {

public:
    CHttpParser();
    virtual HttpMetadata construct(void *Buffer, int len);
    virtual void * deconstruct(HttpMetadata *metadata);
    virtual void logMetadata(HttpMetadata *metadata);
};
