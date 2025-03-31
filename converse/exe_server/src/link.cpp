#include <grpcpp/grpcpp.h>
#include "converse/service/link/link.pb.h"
#include "converse/service/link/link.grpc.pb.h"
#include "link.hpp"

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace converse {
namespace service {
namespace link {

// Global mutex and map to track server information.
static std::mutex gServersMutex;
static std::map<std::string, ServerInfo> gServers;

LinkServiceImpl::LinkServiceImpl() = default;
LinkServiceImpl::~LinkServiceImpl() = default;

grpc::Status LinkServiceImpl::GetServers(
    ::grpc::ServerContext* context,
    const GetServersRequest* request,
    GetServersResponse* response) {
    (void)context;
    (void)request;

    std::lock_guard<std::mutex> lock(gServersMutex);
    // Since the proto defines a repeated uint64 server_ids, we add each server's ID.
    for (const auto& [hostPort, info] : gServers) {
        response->add_server_ids(info.id_assigned ? info.server_id : 0);
    }
    return grpc::Status::OK;
}

grpc::Status LinkServiceImpl::IdentifyMyself(
    ::grpc::ServerContext* context,
    const IdentifyMyselfRequest* request,
    IdentifyMyselfResponse* response) {
    (void)context;
    std::string host = request->host();
    uint32_t port = request->port();

    std::stringstream ss;
    ss << host << ":" << port;
    std::string key = ss.str();

    std::lock_guard<std::mutex> lock(gServersMutex);
    if (gServers.find(key) == gServers.end()) {
        ServerInfo info;
        info.host = host;
        info.port = port;
        // When a server first identifies itself, no ID is assigned (i.e. remains 0)
        gServers[key] = info;
        std::cout << "[IdentifyMyself] Added new server: " << key << "\n";
    } else {
        std::cout << "[IdentifyMyself] Server already known: " << key << "\n";
    }

    // Return 0 until the server claims an ID.
    response->set_server_id(0);
    return grpc::Status::OK;
}

grpc::Status LinkServiceImpl::ClaimServerId(
    ::grpc::ServerContext* context,
    const ClaimServerIdRequest* request,
    ClaimServerIdResponse* response) {
    (void)context;
    // The proto only provides a server_id; we expect unassigned servers to send 0.
    uint64_t temp_id = request->server_id();

    std::lock_guard<std::mutex> lock(gServersMutex);
    // Find a server with a matching (temporary) ID; in our simple model, that will be 0.
    auto it = std::find_if(gServers.begin(), gServers.end(),
                           [temp_id](const auto& pair) {
                              return pair.second.server_id == temp_id;
                           });
    if (it == gServers.end()) {
        std::string err = "[ClaimServerId] Server not found with server_id: " + std::to_string(temp_id);
        std::cerr << err << "\n";
        return grpc::Status(grpc::StatusCode::NOT_FOUND, err);
    }

    ServerInfo& info = it->second;
    if (info.id_assigned && info.server_id != 0) {
        response->set_server_id(info.server_id);
        std::cout << "[ClaimServerId] Server already has ID = " << info.server_id << "\n";
        return grpc::Status::OK;
    }

    // Determine a new unique server id.
    uint64_t maxId = 0;
    for (const auto& [_, s] : gServers) {
        if (s.id_assigned && s.server_id > maxId) {
            maxId = s.server_id;
        }
    }
    uint64_t newId = maxId + 1;
    info.id_assigned = true;
    info.server_id = newId;

    response->set_server_id(newId);
    std::cout << "[ClaimServerId] Assigned server ID = " << newId << "\n";
    return grpc::Status::OK;
}

grpc::Status LinkServiceImpl::ReplicateTransaction(
    ::grpc::ServerContext* context,
    const ReplicateTransactionRequest* request,
    ReplicateTransactionResponse* response) {
    (void)context;

    try {
        for (const auto& op : request->operations()) {
            // Updated to use table_name() as defined in the proto.
            const std::string& table = op.table_name();

            if (op.type() == OPERATION_TYPE_INSERT) {
                std::vector<std::string> columns;
                std::vector<std::string> placeholders;
                std::vector<std::string> values;

                for (const auto& entry : op.new_values()) {
                    columns.push_back(entry.first);
                    placeholders.push_back("?");
                    values.push_back(entry.second);
                }

                std::stringstream sql;
                sql << "INSERT INTO " << table << " (";
                for (size_t i = 0; i < columns.size(); ++i) {
                    sql << columns[i];
                    if (i + 1 < columns.size())
                        sql << ", ";
                }
                sql << ") VALUES (";
                for (size_t i = 0; i < placeholders.size(); ++i) {
                    sql << placeholders[i];
                    if (i + 1 < placeholders.size())
                        sql << ", ";
                }
                sql << ")";

                std::cout << "[SQL:INSERT] " << sql.str() << "\n";
                // Uncomment when integrating with a database:
                // db_->execute(sql.str(), values);
            } else if (op.type() == OPERATION_TYPE_UPDATE) {
                std::vector<std::string> setClauses;
                std::vector<std::string> setValues;
                for (const auto& entry : op.new_values()) {
                    setClauses.push_back(entry.first + " = ?");
                    setValues.push_back(entry.second);
                }

                std::vector<std::string> whereClauses;
                std::vector<std::string> whereValues;
                for (const auto& entry : op.old_key_values()) {
                    whereClauses.push_back(entry.first + " = ?");
                    whereValues.push_back(entry.second);
                }

                std::stringstream sql;
                sql << "UPDATE " << table << " SET ";
                for (size_t i = 0; i < setClauses.size(); ++i) {
                    sql << setClauses[i];
                    if (i + 1 < setClauses.size())
                        sql << ", ";
                }

                if (!whereClauses.empty()) {
                    sql << " WHERE ";
                    for (size_t i = 0; i < whereClauses.size(); ++i) {
                        sql << whereClauses[i];
                        if (i + 1 < whereClauses.size())
                            sql << " AND ";
                    }
                }

                std::vector<std::string> bound;
                bound.insert(bound.end(), setValues.begin(), setValues.end());
                bound.insert(bound.end(), whereValues.begin(), whereValues.end());

                std::cout << "[SQL:UPDATE] " << sql.str() << "\n";
                // Uncomment when integrating with a database:
                // db_->execute(sql.str(), bound);
            } else {
                throw std::runtime_error("Unsupported operation type");
            }
        }
        response->set_success(true);
        return grpc::Status::OK;
    }
    catch (const std::exception& ex) {
        response->set_success(false);
        response->set_error_message(ex.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }
}

}  // namespace link
}  // namespace service
}  // namespace converse
