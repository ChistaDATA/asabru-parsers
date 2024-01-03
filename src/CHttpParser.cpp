
#include <map>
#include "CHttpParser.h"
#include "logger/Logger.h"

void writestringToBuffer(char *buffer, const char *str, size_t &position);

CHttpParser::CHttpParser() {}

HttpMetadata CHttpParser::construct(void *buffer, int len)
{
    LOG_INFO("Constructing Http metadata (start): ");
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
    LOG_INFO("Constructing Http metadata (end): ");
    return result;
}

void *CHttpParser::deconstruct(HttpMetadata *httpMetadata)
{
    LOG_INFO("Deconstructing Http metadata (start): ");
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

    LOG_INFO("Deconstructing Http metadata (end): ");
    return result;
}

void CHttpParser::logMetadata(HttpMetadata *metadata)
{
    // Log the Content and Forward the Data to the EndPoint
    LOG_INFO("=============== CH HTTP ( packet contents ) ===================");
    LOG_INFO("Method: " + metadata->method );
    LOG_INFO("URL: " + metadata->url );
    LOG_INFO("Protocol: " + metadata->protocol );

    LOG_INFO("--------------- Headers Start ---------------------------------:");
    std::map<std::string, std::string> headers = metadata->headers;
    for (auto i = headers.begin(); i != headers.end(); i++)
        std::cout << i->first << "	 " << i->second << std::endl;
    LOG_INFO("---------------  Headers End  ---------------------------------:");

    LOG_INFO("--------------- Body Content ---------------------------------:");
    LOG_INFO(metadata->body);
}
