#include "operations.hpp"

#include <argon2.h>
#include <sqlite3.h>

#include <cstring>
#include <format>
#include <print>
#include <random>
#include <stdexcept>
#include <utility>

#include "database.hpp"

#ifdef PROTOCOL_DYDE
#include "protocols/dyde/server.hpp"
#endif
#ifdef PROTOCOL_JSON

#include "protocols/json/server.hpp"
#endif

#define HASHLEN 32
#define SALTLEN 16

bool verify_password(std::string& password, std::string& password_encoded) {
    uint8_t* pwd = (uint8_t*)strdup(password.c_str());
    uint32_t pwdlen = strlen((char*)pwd);

    int rc = argon2i_verify(password_encoded.c_str(), pwd, pwdlen);
    free(pwd);
    return rc == ARGON2_OK;
}

std::string generate_password_encoded(std::string& password,
                                      std::array<uint8_t, SALTLEN> salt = {0}) {
    if (salt == std::array<uint8_t, SALTLEN>{0}) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        for (size_t i = 0; i < SALTLEN; i++) {
            salt[i] = dis(gen);
        }
    }
    uint8_t* pwd = (uint8_t*)strdup(password.c_str());
    uint32_t pwdlen = strlen((char*)pwd);

    uint32_t t_cost = 2;
    uint32_t m_cost = (1 << 16);
    uint32_t parallelism = 1;

    size_t encodedlen = argon2_encodedlen(t_cost, m_cost, parallelism, SALTLEN,
                                          HASHLEN, Argon2_i);

    std::string encoded(encodedlen, '\0');
    int rc = argon2i_hash_encoded(t_cost, m_cost, parallelism, pwd, pwdlen,
                                  salt.data(), SALTLEN, HASHLEN, encoded.data(),
                                  encodedlen);
    // removes null-terminator
    encoded.pop_back();

    free(pwd);
    if (ARGON2_OK != rc) {
        throw std::runtime_error(
            std::format("argon2 failed: {}", argon2_error_message(rc)));
    }

    return encoded;
}

std::unique_ptr<ClientResponse> handle_client_request(Connection& conn, Db& db,
                                                      Data& data) {
    OperationCode operation_code;
#ifdef PROTOCOL_DYDE
    Data::size_type offset = 0;
    std::tie(operation_code, offset) =
        OperationCodeWrapper::deserialize_from(data, offset);
#endif
#ifdef PROTOCOL_JSON
    std::string data_str(data.begin(), data.end());
    std::printf("Received JSON message: %s\n", data_str.c_str());
    auto obj = boost::json::parse(data_str).as_object();
    operation_code =
        OperationCode(boost::json::value_to<uint8_t>(obj.at("operation_code")));
#endif
    std::printf("operation code: %d\n", static_cast<int>(operation_code));
    try {
        switch (operation_code) {
            case OperationCode::CLIENT_SIGNUP_USER: {
#ifdef PROTOCOL_DYDE
                auto req = ClientSignupUserRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientSignupUserRequest(obj);
#endif
                std::string password_encoded =
                    generate_password_encoded(req.password);
                auto rows = db.execute<sqlite3_int64>(
                    "INSERT INTO users (username, password_encoded) VALUES (?, "
                    "?) RETURNING id",
                    req.username, password_encoded);
                if (rows.empty()) {
                    throw std::runtime_error("failed to create user");
                }
                auto [user_id] = rows.at(0);
                return std::make_unique<ClientSignupUserResponse>(
                    StatusCode::SUCCESS, user_id);
            }
            case OperationCode::CLIENT_SIGNIN_USER: {
#ifdef PROTOCOL_DYDE
                auto req = ClientSigninUserRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientSigninUserRequest(obj);
#endif
                auto rows = db.execute<sqlite3_int64, std::string>(
                    "SELECT id, password_encoded FROM users WHERE username "
                    "= ? AND is_deleted = 0",
                    req.username);
                if (rows.empty()) {
                    throw std::runtime_error("failed to find user");
                }
                auto [id, password_encoded] = rows.at(0);
                if (!verify_password(req.password, password_encoded)) {
                    throw std::runtime_error("incorrect password");
                }
                return std::make_unique<ClientSigninUserResponse>(
                    StatusCode::SUCCESS, id);
            }
            case OperationCode::CLIENT_SIGNOUT_USER: {
#ifdef PROTOCOL_DYDE
                ClientSignoutUserRequest{data, offset};
#endif
#ifdef PROTOCOL_JSON
                ClientSignoutUserRequest{obj};
#endif
                return std::make_unique<ClientSignoutUserResponse>(
                    StatusCode::SUCCESS);
            }
            case OperationCode::CLIENT_DELETE_USER: {
#ifdef PROTOCOL_DYDE
                auto req = ClientDeleteUserRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientDeleteUserRequest(obj);
#endif
                db.execute("BEGIN TRANSACTION");
                sqlite3_int64 user_id = req.user_id;
                try {
                    auto rows = db.execute<std::string>(
                        "SELECT password_encoded FROM users WHERE id = ? AND "
                        "is_deleted = 0",
                        user_id);
                    if (rows.empty()) {
                        throw std::runtime_error("failed to find user");
                    }
                    auto [password_encoded] = rows.at(0);
                    if (!verify_password(req.password, password_encoded)) {
                        throw std::runtime_error("incorrect password");
                    }
                    db.execute("UPDATE users SET is_deleted = 1 WHERE id = ?",
                               user_id);
                    db.execute("COMMIT");
                    return std::make_unique<ClientDeleteUserResponse>(
                        StatusCode::SUCCESS);
                } catch (std::exception& e) {
                    db.execute("ROLLBACK");
                    throw std::runtime_error(
                        std::format("failed to delete user: {}", e.what()));
                }
            }
            case OperationCode::CLIENT_GET_OTHER_USERS: {
#ifdef PROTOCOL_DYDE
                auto req = ClientGetOtherUsersRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientGetOtherUsersRequest(obj);
#endif
                auto rows = db.execute<sqlite3_int64, std::string>(
                    "SELECT id, username FROM users WHERE id != ? AND "
                    "is_deleted = 0 AND username LIKE ? LIMIT ? OFFSET ?",
                    static_cast<sqlite3_int64>(req.user_id),
                    "%" + req.query + "%",
                    static_cast<sqlite3_int64>(req.limit),
                    static_cast<sqlite3_int64>(req.offset));
                return std::make_unique<ClientGetOtherUsersResponse>(
                    StatusCode::SUCCESS,
                    reinterpret_cast<std::vector<std::tuple<Id, String>>&>(
                        rows));
            }
            case OperationCode::CLIENT_CREATE_CONVERSATION: {
#ifdef PROTOCOL_DYDE
                auto req = ClientCreateConversationRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientCreateConversationRequest(obj);
#endif
                sqlite3_int64 send_user_id = req.send_user_id;
                sqlite3_int64 recv_user_id = req.recv_user_id;
                db.execute("BEGIN TRANSACTION");
                try {
                    auto rows = db.execute<sqlite3_int64>(
                        "INSERT INTO conversations DEFAULT VALUES RETURNING "
                        "id");
                    if (rows.empty()) {
                        throw std::runtime_error(
                            "failed to create conversation");
                    }
                    auto [conversation_id] = rows.at(0);
                    db.execute(
                        "INSERT INTO conversations_users (conversation_id, "
                        "user_id) VALUES (?, ?), (?, ?)",
                        conversation_id, send_user_id, conversation_id,
                        recv_user_id);
                    auto rows1 = db.execute<std::string>(
                        "SELECT username FROM users WHERE id = ?",
                        recv_user_id);
                    if (rows1.empty()) {
                        throw std::runtime_error("failed to find user");
                    }
                    auto [recv_user_username] = rows1.at(0);
                    db.execute("COMMIT");
                    return std::make_unique<ClientCreateConversationResponse>(
                        StatusCode::SUCCESS, conversation_id, recv_user_id,
                        recv_user_username);
                } catch (std::exception& e) {
                    db.execute("ROLLBACK");
                    throw std::runtime_error(std::format(
                        "failed to create conversation: {}", e.what()));
                }
            }
            case OperationCode::CLIENT_GET_CONVERSATIONS: {
#ifdef PROTOCOL_DYDE
                auto req = ClientGetConversationsRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientGetConversationsRequest(obj);
#endif
                sqlite3_int64 user_id = req.user_id;
                auto rows = db.execute<sqlite3_int64, sqlite3_int64,
                                       std::string, sqlite3_int64>(
                    "WITH conversation_data AS (SELECT cu.conversation_id, "
                    "cu.user_id AS other_user_id, u.username AS "
                    "other_user_username FROM conversations_users cu JOIN "
                    "users u ON cu.user_id = u.id WHERE cu.conversation_id IN "
                    "(SELECT conversation_id FROM conversations_users WHERE "
                    "user_id = ?) AND cu.user_id != ?) SELECT "
                    "cd.conversation_id, cd.other_user_id, "
                    "cd.other_user_username, COALESCE(COUNT(m.id), 0) AS "
                    "unread_messages FROM conversation_data cd LEFT JOIN "
                    "messages m ON cd.conversation_id = m.conversation_id AND "
                    "m.is_read = 0 AND m.user_id = cd.other_user_id JOIN "
                    "conversations c ON cd.conversation_id = c.id WHERE "
                    "c.is_deleted = 0 GROUP BY cd.conversation_id, "
                    "cd.other_user_id, cd.other_user_username",
                    user_id, user_id);
                std::println("rows: {}", rows.size());
                return std::make_unique<ClientGetConversationsResponse>(
                    StatusCode::SUCCESS,
                    reinterpret_cast<
                        std::vector<std::tuple<Id, Id, String, Size>>&>(rows));
            }
            case OperationCode::CLIENT_DELETE_CONVERSATION: {
#ifdef PROTOCOL_DYDE
                auto req = ClientDeleteConversationRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientDeleteConversationRequest(obj);
#endif
                sqlite3_int64 conversation_id = req.conversation_id;
                db.execute(
                    "UPDATE conversations SET is_deleted = 1 WHERE id = ?",
                    conversation_id);
                return std::make_unique<ClientDeleteConversationResponse>(
                    StatusCode::SUCCESS);
            }
            case OperationCode::CLIENT_SEND_MESSAGE: {
#ifdef PROTOCOL_DYDE
                auto req = ClientSendMessageRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientSendMessageRequest(obj);
#endif
                sqlite3_int64 conversation_id = req.conversation_id;
                sqlite3_int64 send_user_id = req.send_user_id;
                std::string content = req.content;
                db.execute("BEGIN TRANSACTION");
                try {
                    auto rows = db.execute<sqlite3_int64>(
                        "INSERT INTO messages (conversation_id, user_id, "
                        "content) VALUES (?, ?, ?) RETURNING id",
                        conversation_id, send_user_id, content);
                    if (rows.empty()) {
                        throw std::runtime_error("failed to create message");
                    }
                    auto [message_id] = rows.at(0);
                    rows = db.execute<sqlite3_int64>(
                        "SELECT user_id FROM conversations_users WHERE "
                        "conversation_id = ? AND user_id != ?",
                        conversation_id, send_user_id);
                    if (rows.empty()) {
                        throw std::runtime_error("failed to find user");
                    }
                    auto [recv_user_id] = rows.at(0);
                    data = ServerSendMessageRequest(conversation_id, message_id,
                                                    send_user_id, content)
                               .serialize();
                    conn.broadcast(send_user_id, data);
                    conn.broadcast(recv_user_id, data);
                    db.execute("COMMIT");
                    return std::make_unique<ClientSendMessageResponse>(
                        StatusCode::SUCCESS, message_id);
                } catch (std::exception& e) {
                    db.execute("ROLLBACK");
                    std::println("failed to send message: {}", e.what());
                    throw e;
                }
            }
            case OperationCode::CLIENT_GET_MESSAGES: {
#ifdef PROTOCOL_DYDE
                auto req = ClientGetMessagesRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientGetMessagesRequest(obj);
#endif
                sqlite3_int64 conversation_id = req.conversation_id;
                std::println("conversation_id: {}", conversation_id);
                auto rows =
                    db.execute<sqlite3_int64, sqlite3_int64, bool, std::string>(
                        "SELECT id, user_id, is_read, content FROM messages "
                        "WHERE conversation_id = ? AND is_deleted = 0 ORDER BY "
                        "created_at ASC",
                        conversation_id);
                return std::make_unique<ClientGetMessagesResponse>(
                    StatusCode::SUCCESS,
                    reinterpret_cast<
                        std::vector<std::tuple<Id, Id, Bool, String>>&>(rows));
            }
            case OperationCode::CLIENT_DELETE_MESSAGE: {
#ifdef PROTOCOL_DYDE
                auto req = ClientDeleteMessageRequest(data, offset);
#endif
#ifdef PROTOCOL_JSON
                auto req = ClientDeleteMessageRequest(obj);
#endif
                sqlite3_int64 conversation_id = req.conversation_id;
                sqlite3_int64 message_id = req.message_id;
                db.execute(
                    "UPDATE messages SET is_deleted = 1 WHERE conversation_id "
                    "= ? AND id = ?",
                    conversation_id, message_id);
                return std::make_unique<ClientDeleteMessageResponse>(
                    StatusCode::SUCCESS);
            }
            default: {
                throw std::runtime_error("invalid operation code");
            }
        }
    } catch (std::exception& e) {
        std::println("operation {} error: {}",
                     std::to_underlying(operation_code), e.what());
        return std::make_unique<ClientResponse>(operation_code,
                                                StatusCode::FAILURE);
    }
}
