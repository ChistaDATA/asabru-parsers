
#include <map>
#include "CHttpParser.h"

void writestringToBuffer(char *buffer, const char *str, size_t &position);

CHttpParser::CHttpParser() {}

HttpMetadata CHttpParser::construct(void *buffer, int len)
{
    std::cout << "Constructing Http metadata (start): " << std::endl;
    HttpMetadata result;
    LineGrabber lineGrabber((char *)buffer, len);

    // Grab the first line for method, url and protocol
    std::string first_line = lineGrabber.getNextLine();
    char first_line_copy[1024];
    ::strcpy(first_line_copy, first_line.c_str());
    char method[4096];
    char url[4096];
    char protocol[4096];
    sscanf(first_line_copy, "%s %s %s", method, url, protocol);
    result.url = url;
    result.protocol = protocol;
    result.method = method;

    // Grab the headers
    while (!lineGrabber.isEof())
    {
        std::string test = lineGrabber.getNextLine();
        if (strlen(test.c_str()) == 0)
        {
            break;
        }
        std::pair<std::string, std::string> temp = Utils::ChopLine(test);
        result.headers.insert(temp);
    }

    // Rest of the content will be the body of the packet
    std::string body = lineGrabber.getContentTillEof();
    result.body = body;
    std::cout << "Constructing Http metadata (end): " << std::endl;
    return result;
}

void *CHttpParser::deconstruct(HttpMetadata *httpMetadata)
{
    std::cout << "Deconstructing Http metadata (start): " << std::endl;
    std::string buffer = "";
    buffer += (*httpMetadata).method + " " + (*httpMetadata).url + " " + (*httpMetadata).protocol + "\r\n";

    std::map<std::string, std::string> *headers = &((*httpMetadata).headers);
    for (auto i : *headers)
    {
        buffer += i.first + ":" + i.second + "\r\n";
    }

    buffer += "\r\n";
    buffer += (*httpMetadata).body;

    size_t buffer_length = strlen(buffer.c_str());
    char *result = (char *)malloc(buffer_length);
    memcpy(result, buffer.c_str(), buffer_length);

    std::cout << "Deconstructing Http metadata (end): " << std::endl;
    return result;
}

void CHttpParser::logMetadata(HttpMetadata *metadata)
{
    // Log the Content and Forward the Data to the EndPoint
    std::cout << "=============== CH HTTP ( packet contents ) ===================" << std::endl;
    std::cout << "Method: " << metadata->method << std::endl;
    std::cout << "URL: " << metadata->url << std::endl;
    std::cout << "Protocol: " << metadata->protocol << std::endl;

    std::cout << "--------------- Headers Start ---------------------------------:" << std::endl;
    std::map<std::string, std::string> headers = metadata->headers;
    for (auto i = headers.begin(); i != headers.end(); i++)
        std::cout << i->first << "	 " << i->second << std::endl;
    std::cout << "---------------  Headers End  ---------------------------------:" << std::endl;

    std::cout << "--------------- Body Content ---------------------------------:" << std::endl;
    std::cout << metadata->body << std::endl;
}
