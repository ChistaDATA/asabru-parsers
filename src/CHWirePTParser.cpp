//
// Created by Midhun Darvin on 14/03/24.
//

#include "CHWirePTParser.h"

CHWirePTParser::CHWirePTParser() {}

void CHWirePTParser::LogResponse(char * buffer, int len)
{
    uint64_t packet_type = 0;
    uint64_t readBytes = 0;
    char * temp = buffer;

    readBytes = readVarUInt(packet_type, temp);

    this->parsePacket(temp, len, "Server");
}

uint64_t CHWirePTParser::parseClientHello(char * stream, uint64_t len)
{
    uint64_t packet_type = 0;
    uint64_t bytesRead = 0;

    bytesRead += readVarUInt(packet_type, (stream + bytesRead));
    if (bytesRead == len)
        return bytesRead;
    bytesRead += readStringBinary(packet.client_name, (stream + bytesRead));
    bytesRead += readVarUInt(packet.version_major, (stream + bytesRead));
    bytesRead += readVarUInt(packet.version_minor, (stream + bytesRead));
    bytesRead += readVarUInt(packet.protocol_version, (stream + bytesRead));
    bytesRead += readStringBinary(packet.database, (stream + bytesRead));
    bytesRead += readStringBinary(packet.username, (stream + bytesRead));
    bytesRead += readStringBinary(packet.password, (stream + bytesRead));
    if (packet.username == USER_INTERSERVER_MARKER)
    {
        packet.interserverMode = true;
        bytesRead += readStringBinary(packet.cluster, stream + bytesRead);
        bytesRead += readStringBinary(packet.salt, stream + bytesRead, 32);
    }


    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "Client || Packet-Type: " << packet_type << " - "
         << "HELLO" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "client_name: " << packet.client_name << std::endl;
    std::cout << "version_major: " << packet.version_major << std::endl;
    std::cout << "version_minor: " << packet.version_minor << std::endl;
    std::cout << "protocol_version: " << packet.protocol_version << std::endl;
    std::cout << "database: " << packet.database << std::endl;
    std::cout << "username: " << packet.username << std::endl;
    std::cout << "password: " << packet.password << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    return bytesRead;
}

QueryType CHWirePTParser::parseClientQuery(char * stream, uint64_t len, EXECUTION_CONTEXT *exec_context)
{
    std::string correlation_id = (char *) (*exec_context)["correlation_id"];
    ClientQueryPacket queryPacket;
    uint64_t packet_type = 0;
    uint8_t queryKind = 0;
    uint8_t interface = 0;
    uint8_t stage = 0;
    uint64_t bytesRead = 0;

    bytesRead += readVarUInt(packet_type, (stream + bytesRead));

    if (bytesRead == len)
        return QueryType::Other;

    bytesRead += readStringBinary(queryPacket.query_id, (stream + bytesRead));
    if (this->packet.protocol_version >= DBMS_MIN_REVISION_WITH_CLIENT_INFO)
    {
        bytesRead += readBinary(queryKind, (stream + bytesRead));
        queryPacket.client_info.query_kind = QueryKind(queryKind);
        if (queryPacket.client_info.query_kind != QueryKind::NO_QUERY)
        {
            bytesRead += readStringBinary(queryPacket.client_info.initial_user, (stream + bytesRead));
            bytesRead += readStringBinary(queryPacket.client_info.initial_query_id, (stream + bytesRead));
            bytesRead += readStringBinary(queryPacket.client_info.initial_address, (stream + bytesRead));
            if (packet.protocol_version >= DBMS_MIN_PROTOCOL_VERSION_WITH_INITIAL_QUERY_START_TIME)
            {
                bytesRead += readPODBinary(queryPacket.client_info.initial_query_start_time_microseconds, (stream + bytesRead));
            }
            bytesRead += readBinary(interface, (stream + bytesRead));
            queryPacket.client_info.interface = Interface(interface);

            bytesRead += readStringBinary(queryPacket.client_info.os_user, (stream + bytesRead));
            bytesRead += readStringBinary(queryPacket.client_info.client_hostname, (stream + bytesRead));
            bytesRead += readStringBinary(queryPacket.client_info.client_name, (stream + bytesRead));
            bytesRead += readVarUInt(queryPacket.client_info.client_version_major, (stream + bytesRead));
            bytesRead += readVarUInt(queryPacket.client_info.client_version_minor, (stream + bytesRead));
            bytesRead += readVarUInt(queryPacket.client_info.client_revision, (stream + bytesRead));

            if (packet.protocol_version >= DBMS_MIN_REVISION_WITH_QUOTA_KEY_IN_CLIENT_INFO)
                bytesRead += readStringBinary(queryPacket.client_info.quota_key, (stream + bytesRead));
            if (packet.protocol_version >= DBMS_MIN_PROTOCOL_VERSION_WITH_DISTRIBUTED_DEPTH)
                bytesRead += readVarUInt(queryPacket.client_info.distributed_depth, (stream + bytesRead));

            if (packet.protocol_version >= DBMS_MIN_REVISION_WITH_VERSION_PATCH)
                bytesRead += readVarUInt(queryPacket.client_info.client_version_patch, (stream + bytesRead));
            else
                queryPacket.client_info.client_version_patch = queryPacket.client_info.client_revision;

            if (packet.protocol_version >= DBMS_MIN_REVISION_WITH_OPENTELEMETRY)
            {
                uint8_t have_trace_id = 0;
                bytesRead += readBinary(have_trace_id, (stream + bytesRead));
                if (have_trace_id)
                {
                    bytesRead += readBinary(queryPacket.client_info.client_trace_context.trace_id, (stream + bytesRead));
                    bytesRead += readBinary(queryPacket.client_info.client_trace_context.span_id, (stream + bytesRead));
                    bytesRead += readBinary(queryPacket.client_info.client_trace_context.tracestate, (stream + bytesRead));
                    bytesRead += readBinary(queryPacket.client_info.client_trace_context.trace_flags, (stream + bytesRead));
                }
            }
            if (packet.protocol_version >= DBMS_MIN_REVISION_WITH_PARALLEL_REPLICAS)
            {
                uint64_t value = 0;
                bytesRead += readVarUInt(value, (stream + bytesRead));
                queryPacket.client_info.collaborate_with_initiator = static_cast<bool>(value);
                bytesRead += readVarUInt(queryPacket.client_info.count_participating_replicas, (stream + bytesRead));
                bytesRead += readVarUInt(queryPacket.client_info.number_of_current_replica, (stream + bytesRead));
            }
        }
    }

    auto settings_format = (this->packet.protocol_version >= DBMS_MIN_REVISION_WITH_SETTINGS_SERIALIZED_AS_STRINGS)
                           ? SettingsWriteFormat::STRINGS_WITH_FLAGS
                           : SettingsWriteFormat::BINARY;
    bytesRead += readBaseSettings(stream + bytesRead, settings_format, this->packet.protocol_version, &queryPacket.settings);

    std::string received_hash;
    if (this->packet.protocol_version >= DBMS_MIN_REVISION_WITH_INTERSERVER_SECRET)
    {
        bytesRead += readStringBinary(received_hash, stream + bytesRead, 32);
    }

    bytesRead += readBinary(stage, (stream + bytesRead));
    queryPacket.stage = Stage(stage);

    bytesRead += readVarUInt(queryPacket.compression, (stream + bytesRead));

    bytesRead += readStringBinary(queryPacket.query, (stream + bytesRead));

    queryPacket.queryType = findQueryType(queryPacket.query);

    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "Client || Packet-Type: " << packet_type << " - "
         << "QUERY" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "Initial Query Id: " << queryPacket.query_id << std::endl;
    std::cout << "ClientInfo -> Query Kind: " << std::to_string(static_cast<uint8_t>(queryPacket.client_info.query_kind)) << std::endl;
    std::cout << "ClientInfo -> Initial User: " << queryPacket.client_info.initial_user << std::endl;
    std::cout << "ClientInfo -> Initial Query Id: " << queryPacket.client_info.initial_query_id << std::endl;
    std::cout << "ClientInfo -> Initial Address: " << queryPacket.client_info.initial_address << std::endl;
    std::cout << "ClientInfo -> Interface: " << std::to_string(static_cast<uint8_t>(queryPacket.client_info.interface)) << std::endl;
    std::cout << "ClientInfo -> OS User: " << queryPacket.client_info.os_user << std::endl;
    std::cout << "ClientInfo -> Client Hostname: " << queryPacket.client_info.client_hostname << std::endl;
    std::cout << "ClientInfo -> Client Name: " << queryPacket.client_info.client_name << std::endl;
    std::cout << "ClientInfo -> Client Version Major: " << queryPacket.client_info.client_version_major << std::endl;
    std::cout << "ClientInfo -> Client Version Minor: " << queryPacket.client_info.client_version_minor << std::endl;
    std::cout << "ClientInfo -> Client Revision: " << queryPacket.client_info.client_revision << std::endl;
    std::cout << "ClientInfo -> Client Quota Key: " << queryPacket.client_info.quota_key << std::endl;
    std::cout << "ClientInfo -> Client Version Patch: " << queryPacket.client_info.client_version_patch << std::endl;
    std::cout << "Secret: " << queryPacket.secret << std::endl;
    std::cout << "Stage: " << std::to_string(static_cast<uint8_t>(queryPacket.stage)) << std::endl;
    std::cout << "Compression: " << queryPacket.compression << std::endl;
    std::cout << "Query: " << queryPacket.query << std::endl;
    LOG_QUERY(correlation_id, queryPacket.query)
    std::cout << "QueryType: " << std::to_string(static_cast<uint8_t>(queryPacket.queryType)) << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;

    return queryPacket.queryType;
}

uint64_t CHWirePTParser::parsePacket(char * stream, int len, std::string source)
{
    uint64_t packet_type = 0;
    uint64_t bytesRead = 0;

    bytesRead += readVarUInt(packet_type, (stream + bytesRead));

    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << source << " || Packet-Type: " << packet_type << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    int s = 0;
    while (s < len)
    {
        std::cout << *stream++;
        s++;
    }
    std::cout << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    return bytesRead;
}
