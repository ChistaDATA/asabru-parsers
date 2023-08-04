#pragma  once
#include <map>
#include <string>

using namespace std;

struct HttpMetadata {
    std::string url;
    std::string method;
    std::string protocol;
    std::string body;
    map<std::string, std::string> headers;
};

pair<std::string, std::string> ChopLine(std::string str);

class CHttpParser {

public:
    CHttpParser();
    virtual HttpMetadata construct(void *Buffer, int len);
    virtual void * deconstruct(HttpMetadata *metadata);
    virtual void logMetadata(HttpMetadata *metadata);
};
