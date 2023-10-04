#pragma once

#include <map>
#include <string>
#include "Utils.h"
#include "LineGrabber.h"
#include "CommonTypes.h"
class CHttpParser {

public:
    CHttpParser();
    virtual HttpMetadata construct(void *Buffer, int len);
    virtual void * deconstruct(HttpMetadata *metadata);
    virtual void logMetadata(HttpMetadata *metadata);
};
