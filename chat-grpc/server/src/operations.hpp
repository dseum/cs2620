#pragma once

#include <memory>

#include "connections.hpp"
#include "database.hpp"
#include "protocols/utils.hpp"
#ifdef PROTOCOL_DYDE
#include "protocols/dyde/client.hpp"
#endif
#ifdef PROTOCOL_JSON
#include "protocols/json/client.hpp"
#endif

// std::unique_ptr<ClientResponse> handle_client_request(Connection& conn, Db& db,
//                                                       Data& data);
