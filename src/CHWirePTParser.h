#pragma once

#include <map>
#include <regex>
#include <string>
#include <utility>
#include <list>
#include "Utils.h"
#include "LineGrabber.h"
#include "CommonTypes.h"
#include "Logger.h"

#define DBMS_MIN_REVISION_WITH_CLIENT_INFO 54032
#define DBMS_MIN_REVISION_WITH_QUOTA_KEY_IN_CLIENT_INFO 54060
#define DBMS_MIN_REVISION_WITH_VERSION_PATCH 54401
#define DBMS_MIN_REVISION_WITH_SETTINGS_SERIALIZED_AS_STRINGS 54429
#define DBMS_MIN_REVISION_WITH_OPENTELEMETRY 54442
#define DBMS_MIN_REVISION_WITH_PARALLEL_REPLICAS 54453

/// Minimum revision supporting interserver secret.
#define DBMS_MIN_REVISION_WITH_INTERSERVER_SECRET 54441
#define DBMS_MIN_PROTOCOL_VERSION_WITH_DISTRIBUTED_DEPTH 54448

#define DBMS_MIN_PROTOCOL_VERSION_WITH_INITIAL_QUERY_START_TIME 54449

const char USER_INTERSERVER_MARKER[] = " INTERSERVER SECRET ";

enum class QueryType : uint8_t
{
    Read = 0,
    Write = 1,
    Other = 2
};

enum class QueryKind : uint8_t
{
    NO_QUERY = 0, /// Uninitialized object.
    INITIAL_QUERY = 1,
    SECONDARY_QUERY = 2, /// Query that was initiated by another query for distributed or ON CLUSTER query execution.
};

enum class Interface : uint8_t
{
    TCP = 1,
    HTTP = 2,
};

enum class Stage : uint8_t
{
    FetchColumns = 0,
    WithMergeableState = 1,
    Complete = 2,
};

struct ClientTraceContext
{
    uint8_t trace_id;
    uint8_t span_id;
    uint8_t tracestate;
    uint8_t trace_flags;
};

enum class SettingsWriteFormat
{
    BINARY, /// Part of the settings are serialized as strings, and other part as variants. This is the old behaviour.
    STRINGS_WITH_FLAGS, /// All settings are serialized as strings. Before each value the flag `is_important` is serialized.
    DEFAULT = STRINGS_WITH_FLAGS,
};

struct ClientInfoPacket
{
    QueryKind query_kind;
    std::string initial_user;
    std::string initial_query_id;
    std::string initial_address;
    Interface interface;
    std::string os_user;
    std::string client_hostname;
    std::string client_name;
    uint64_t client_version_major;
    uint64_t client_version_minor;
    uint64_t client_revision;
    std::string quota_key;
    uint64_t client_version_patch;
    int64_t initial_query_start_time_microseconds;
    uint64_t distributed_depth;
    bool collaborate_with_initiator;
    uint64_t count_participating_replicas;
    uint64_t number_of_current_replica;
    ClientTraceContext client_trace_context;
};

struct ClientHelloPacket
{
    std::string client_name;
    uint64_t version_major;
    uint64_t version_minor;
    uint64_t protocol_version;
    std::string database;
    std::string username;
    std::string password;
    bool interserverMode;
    std::string cluster;
    std::string salt;
};

struct ClientQueryPacket
{
    std::string query_id;
    ClientInfoPacket client_info;
    std::string secret;
    Stage stage;
    uint64_t compression;
    std::string query;
    std::list<std::pair<std::string, std::string>> settings;
    QueryType queryType;
};

template <typename T>
uint64_t readPODBinary(T & x, char * stream)
{
    x = *reinterpret_cast<const T *>(stream);
    return sizeof(x);
}

static uint64_t readBinary(uint8_t & x, char * stream)
{
    x = stream[0];
    return 1;
}

static uint64_t readVarUInt(uint64_t & x, char * stream)
{
    x = 0;
    size_t i = 0;
    for (; i < 9; ++i)
    {
        uint64_t byte = stream[0]; /// NOLINT
        stream++;
        x |= (byte & 0x7F) << (7 * i);

        if (!(byte & 0x80))
            return (i + 1);
    }
    return (i + 1);
}

static uint64_t readStringBinary(std::string & s, char * stream)
{
    uint64_t size = 0;
    uint64_t bytesRead = readVarUInt(size, stream++);

    for (int i = 0; i < size; i++, stream++)
    {
        if (reinterpret_cast<const char *>(*stream) != NULL)
        {
            s += *stream;
        }
    }

    return (size + bytesRead);
}

static uint64_t readStringBinary(std::string & s, char * stream, size_t MAX_STRING_SIZE)
{
    uint64_t size = 0;
    uint64_t bytesRead = readVarUInt(size, stream);

    if (size > MAX_STRING_SIZE)
        throw "Too large string size.";

    for (int i = 0; i < size; i++, stream++)
    {
        if (reinterpret_cast<const char *>(*stream) != NULL)
        {
            s += *stream;
        }
    }

    return (size + bytesRead);
}

const std::string WHITESPACE =
        //        " \n\r\t\f\v";
        " \s\t\n\f\r\v\p{Z}";


static std::string trim(std::string str)
{
    str.erase(str.find_last_not_of(WHITESPACE) + 1);
    str.erase(0, str.find_first_not_of(WHITESPACE));
    return str;
}

const std::regex ReadQueryRegex("^.*?(select |show ).*", std::regex_constants::icase);
const std::regex WriteQueryRegex("^.*?(insert |update |alter ).*", std::regex_constants::icase);

static QueryType findQueryType(std::string query)
{
    query = trim(query);
    if (regex_match(query.begin(), query.end(), ReadQueryRegex))
        return QueryType::Read;
    if (regex_match(query.begin(), query.end(), WriteQueryRegex))
        return QueryType::Write;
    return QueryType::Other;
}

static uint64_t readBaseSettings(char * stream, SettingsWriteFormat format, uint64_t protocol_version, std::list<std::pair<std::string, std::string>> * settings)
{
    if (protocol_version < DBMS_MIN_REVISION_WITH_CLIENT_INFO)
        throw("Logical error: method ClientInfo::read is called for unsupported client revision");
    uint64_t bytesRead = 0;
    while (true)
    {
        std::string name;
        bytesRead += readStringBinary(name, stream + bytesRead);
        if (name.empty() /* empty string is a marker of the end of settings */)
            break;
        uint64_t flags;
        if (format >= SettingsWriteFormat::STRINGS_WITH_FLAGS)
            bytesRead += readVarUInt(flags, stream + bytesRead);
        // bool is_important = (flags & BaseSettingsFlags::IMPORTANT);
        // bool is_custom = (flags & BaseSettingsFlags::CUSTOM);

        std::string value;
        bytesRead += readStringBinary(value, stream + bytesRead);
        settings->push_back(make_pair(name, value));
        std::cout << "BaseSettings: name: " << name << " , value: " << value << std::endl;
    }
    return bytesRead;
}

class CHWirePTParser {
public:
    CHWirePTParser();

    ClientHelloPacket packet;
    void LogResponse(char * buffer, int len, std::string type);
    uint64_t parsePacket(char * stream, int len, std::string source);
    uint64_t parseClientHello(char * stream, uint64_t len);
    QueryType parseClientQuery(char * stream, uint64_t len, EXECUTION_CONTEXT *exec_context);
};
