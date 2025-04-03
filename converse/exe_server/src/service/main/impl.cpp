#include "service/main/impl.hpp"

#include <argon2.h>
#include <converse/service/link/link.pb.h>
#include <converse/service/main/main.pb.h>
#include <grpcpp/server_context.h>

#include <converse/logging/core.hpp>
#include <cstring>
#include <format>
#include <random>
#include <stdexcept>

namespace lg = converse::logging;

bool verify_password(const std::string &password,
                     const std::string &password_encoded) {
    uint8_t *pwd = (uint8_t *)strdup(password.c_str());
    uint32_t pwdlen = static_cast<uint32_t>(strlen((char *)pwd));

    int rc = argon2i_verify(password_encoded.c_str(), pwd, pwdlen);
    free(pwd);
    return rc == ARGON2_OK;
}

std::string generate_password_encoded(const std::string &password,
                                      std::array<uint8_t, SALTLEN> salt) {
    if (salt == std::array<uint8_t, SALTLEN>{0}) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        for (size_t i = 0; i < SALTLEN; i++) {
            salt[i] = dis(gen);
        }
    }
    uint8_t *pwd = (uint8_t *)strdup(password.c_str());
    uint32_t pwdlen = static_cast<uint32_t>(strlen((char *)pwd));

    uint32_t t_cost = 2;
    uint32_t m_cost = (1 << 16);
    uint32_t parallelism = 1;

    size_t encodedlen = argon2_encodedlen(t_cost, m_cost, parallelism, SALTLEN,
                                          HASHLEN, Argon2_i);

    std::string encoded(encodedlen, '\0');
    int rc = argon2i_hash_encoded(t_cost, m_cost, parallelism, pwd, pwdlen,
                                  salt.data(), SALTLEN, HASHLEN, encoded.data(),
                                  encodedlen);

    // remove trailing null terminator if present
    if (!encoded.empty() && encoded.back() == '\0') {
        encoded.pop_back();
    }

    free(pwd);
    if (ARGON2_OK != rc) {
        throw std::runtime_error(
            std::format("argon2 failed: {}", argon2_error_message(rc)));
    }

    return encoded;
}

namespace converse {
namespace service {
namespace main {

Impl::Impl(const std::string &name, const server::Address &address,
           std::set<grpc::ServerWriter<link::GetTransactionsResponse> *>
               &cluster_writers,
           std::shared_mutex &cluster_writers_mutex)
    : name_(name),
      address_(address),
      db_(std::make_unique<Db>(name_)),
      cluster_writers_(cluster_writers),
      cluster_writers_mutex_(cluster_writers_mutex) {}

Impl::~Impl() = default;

grpc::Status Impl::SignupUser(grpc::ServerContext *context,
                              const SignupUserRequest *request,
                              SignupUserResponse *response) {
    try {
        size_t first = request->user_username().find_first_not_of(' ');
        size_t last = request->user_username().find_last_not_of(' ');
        std::string username = request->user_username().substr(first, last + 1);
        if (username.empty()) {
            throw std::runtime_error("username is empty");
        }
        if (username.size() > 255) {
            throw std::runtime_error("username is too long");
        }
        if (request->user_password().empty()) {
            throw std::runtime_error("password is empty");
        }
        std::string password_encoded =
            generate_password_encoded(request->user_password());

        auto rows = db_->execute<sqlite3_int64>(
            "INSERT INTO users (username, password_encoded) VALUES (?, ?) "
            "RETURNING id",
            request->user_username(), password_encoded);

        if (rows.empty()) {
            throw std::runtime_error("failed to create user");
        }

        auto &[id] = rows.at(0);
        response->set_user_id(id);

        {
            link::GetTransactionsResponse response;
            auto *op = response.add_operations();
            op->set_type(link::OPERATION_TYPE_INSERT);
            op->set_table_name("users");
            (*op->mutable_new_values())["id"] = std::to_string(id);
            (*op->mutable_new_values())["username"] = username;
            (*op->mutable_new_values())["password_encoded"] = password_encoded;

            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }

        lg::write(lg::level::info, "SignupUser({}) -> Ok({})", username, id);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "SignupUser({}) -> Err({})",
                  request->user_username(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::SigninUser(grpc::ServerContext *context,
                              const SigninUserRequest *request,
                              SigninUserResponse *response) {
    try {
        auto rows = db_->execute<sqlite3_int64, std::string>(
            "SELECT id, password_encoded FROM users "
            "WHERE username = ? AND is_deleted = 0",
            request->user_username());
        if (rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto &[id, password_encoded] = rows.at(0);
        if (!verify_password(request->user_password(), password_encoded)) {
            throw std::runtime_error("incorrect password");
        }
        response->set_user_id(id);

        lg::write(lg::level::info, "SigninUser({}) -> Ok({})",
                  request->user_username(), id);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "SigninUser({}) -> Err({})",
                  request->user_username(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::SignoutUser(grpc::ServerContext *context,
                               const SignoutUserRequest *request,
                               SignoutUserResponse *response) {
    lg::write(lg::level::info, "SignoutUser({}) -> Ok()", request->user_id());
    return grpc::Status::OK;
}

grpc::Status Impl::DeleteUser(grpc::ServerContext *context,
                              const DeleteUserRequest *request,
                              DeleteUserResponse *response) {
    try {
        db_->execute("BEGIN TRANSACTION");
        sqlite3_int64 user_id = request->user_id();
        auto rows = db_->execute<std::string>(
            "SELECT password_encoded FROM users WHERE id = ? AND is_deleted = "
            "0",
            user_id);
        if (rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [password_encoded] = rows.at(0);
        if (!verify_password(request->user_password(), password_encoded)) {
            throw std::runtime_error("incorrect password");
        }
        db_->execute("UPDATE users SET is_deleted = 1 WHERE id = ?", user_id);
        db_->execute("COMMIT");

        {
            link::GetTransactionsResponse response;
            auto *op = response.add_operations();
            op->set_type(link::OPERATION_TYPE_UPDATE);
            op->set_table_name("users");
            (*op->mutable_new_values())["is_deleted"] = "1";
            (*op->mutable_old_key_values())["id"] = std::to_string(user_id);

            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }

        lg::write(lg::level::info, "DeleteUser({}) -> Ok()", user_id);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        db_->execute("ROLLBACK");
        lg::write(lg::level::error, "DeleteUser({}) -> Err({})",
                  request->user_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::GetOtherUsers(grpc::ServerContext *context,
                                 const GetOtherUsersRequest *request,
                                 GetOtherUsersResponse *response) {
    try {
        sqlite3_int64 user_id = request->user_id();
        std::string like_pattern = "%" + request->query() + "%";
        auto rows = db_->execute<sqlite3_int64, std::string>(
            "SELECT id, username FROM users "
            "WHERE id != ? AND is_deleted = 0 AND username LIKE ? "
            "LIMIT ? OFFSET ?",
            user_id, like_pattern, static_cast<sqlite3_int64>(request->limit()),
            static_cast<sqlite3_int64>(request->offset()));

        for (const auto &[id, username] : rows) {
            auto *user = response->add_users();
            user->set_id(id);
            user->set_username(username);
        }
        lg::write(lg::level::info, "GetOtherUsers({}) -> Ok({})", user_id,
                  rows.size());
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "GetOtherUsers({}) -> Err({})",
                  request->user_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::CreateConversation(grpc::ServerContext *context,
                                      const CreateConversationRequest *request,
                                      CreateConversationResponse *response) {
    try {
        sqlite3_int64 send_user_id = request->user_id();
        sqlite3_int64 recv_user_id = request->other_user_id();

        db_->execute("BEGIN TRANSACTION");
        std::string recv_user_username;
        {
            auto rows = db_->execute<std::string>(
                "SELECT username FROM users WHERE id = ? AND is_deleted = 0",
                recv_user_id);
            if (rows.empty()) {
                throw std::runtime_error("failed to find user");
            }
            recv_user_username = std::move(std::get<0>(rows.at(0)));
        }
        sqlite3_int64 conversation_id;
        {
            auto rows = db_->execute<sqlite3_int64>(
                "INSERT INTO conversations DEFAULT VALUES RETURNING id");
            if (rows.empty()) {
                throw std::runtime_error("conversation not inserted");
            }
            conversation_id = std::get<0>(rows.at(0));
        }
        db_->execute(
            "INSERT INTO conversations_users (conversation_id, user_id) "
            "VALUES (?, ?), (?, ?)",
            conversation_id, send_user_id, conversation_id, recv_user_id);
        db_->execute("COMMIT");

        {
            link::GetTransactionsResponse response;
            {
                auto *op = response.add_operations();
                op->set_type(link::OPERATION_TYPE_INSERT);
                op->set_table_name("conversations");
                (*op->mutable_new_values())["id"] =
                    std::to_string(conversation_id);
            }
            {
                auto *op = response.add_operations();
                op->set_type(link::OPERATION_TYPE_INSERT);
                op->set_table_name("conversations_users");
                (*op->mutable_new_values())["conversation_id"] =
                    std::to_string(conversation_id);
                (*op->mutable_new_values())["user_id"] =
                    std::to_string(send_user_id);
            }
            {
                auto *op = response.add_operations();
                op->set_type(link::OPERATION_TYPE_INSERT);
                op->set_table_name("conversations_users");
                (*op->mutable_new_values())["conversation_id"] =
                    std::to_string(conversation_id);
                (*op->mutable_new_values())["user_id"] =
                    std::to_string(recv_user_id);
            }
            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }

        Conversation *conversation = new Conversation();
        conversation->set_id(conversation_id);
        conversation->set_recv_user_id(recv_user_id);
        conversation->set_recv_user_username(recv_user_username);
        response->set_allocated_conversation(conversation);

        lg::write(lg::level::info, "CreateConversation({},{}) -> Ok({},{},{})",
                  send_user_id, recv_user_id, conversation_id, recv_user_id,
                  recv_user_username);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        db_->execute("ROLLBACK");
        lg::write(lg::level::error, "CreateConversation({},{}) -> Err({})",
                  request->user_id(), request->other_user_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::GetConversation(grpc::ServerContext *context,
                                   const GetConversationRequest *request,
                                   GetConversationResponse *response) {
    try {
        sqlite3_int64 user_id = request->user_id();
        sqlite3_int64 conversation_id = request->conversation_id();
        auto rows = db_->execute<sqlite3_int64, std::string, std::string>(
            R"(
            SELECT cu.user_id,
                   u.username,
                   GROUP_CONCAT(m.id) AS unread_message_ids
            FROM conversations_users cu
            JOIN users u ON cu.user_id = u.id
            LEFT JOIN messages m ON cu.conversation_id = m.conversation_id
                                AND m.user_id = cu.user_id
                                AND m.is_read = 0
            WHERE cu.conversation_id = ? AND cu.user_id != ?
            GROUP BY cu.conversation_id, cu.user_id, u.username
            )",
            conversation_id, user_id);
        if (rows.empty()) {
            throw std::runtime_error("failed to find conversation");
        }
        auto &[recv_user_id, recv_user_username, unread_message_ids] =
            rows.at(0);
        Conversation *conversation = new Conversation();
        conversation->set_id(conversation_id);
        conversation->set_recv_user_id(recv_user_id);
        conversation->set_recv_user_username(recv_user_username);
        for (const auto &unread_message_id :
             unread_message_ids | std::views::split(',') |
                 std::views::transform([](auto &&range) {
                     return std::stoll(std::string(range.begin(), range.end()));
                 })) {
            conversation->add_unread_message_ids(unread_message_id);
        }
        response->set_allocated_conversation(conversation);

        lg::write(lg::level::info, "GetConversation({},{}) -> Ok({},{},{})",
                  request->user_id(), request->conversation_id(),
                  conversation_id, recv_user_id, recv_user_username);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "GetConversation({},{}) -> Err({})",
                  request->user_id(), request->conversation_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::GetConversations(grpc::ServerContext *context,
                                    const GetConversationsRequest *request,
                                    GetConversationsResponse *response) {
    try {
        sqlite3_int64 user_id = request->user_id();
        auto rows = db_->execute<sqlite3_int64, sqlite3_int64, std::string,
                                 std::string>(
            R"(
            WITH conversation_data AS (
              SELECT cu.conversation_id,
                     cu.user_id AS other_user_id,
                     u.username AS other_user_username
              FROM conversations_users cu
              JOIN users u ON cu.user_id = u.id
              WHERE cu.conversation_id IN (
                  SELECT conversation_id
                  FROM conversations_users
                  WHERE user_id = ?
              )
              AND cu.user_id != ?
              AND u.is_deleted = 0
            )
            SELECT cd.conversation_id,
                   cd.other_user_id,
                   cd.other_user_username,
                   GROUP_CONCAT(m.id) AS unread_message_ids
            FROM conversation_data cd
            LEFT JOIN messages m
              ON cd.conversation_id = m.conversation_id
             AND m.is_read = 0
             AND m.user_id = cd.other_user_id
            JOIN conversations c ON cd.conversation_id = c.id
            WHERE c.is_deleted = 0
            GROUP BY cd.conversation_id,
                     cd.other_user_id,
                     cd.other_user_username
            )",
            user_id, user_id);

        for (auto &[id, recv_user_id, recv_user_username, unread_message_ids] :
             rows) {
            auto *conv = response->add_conversations();
            conv->set_id(id);
            conv->set_recv_user_id(recv_user_id);
            conv->set_recv_user_username(recv_user_username);

            auto result =
                unread_message_ids | std::views::split(',') |
                std::views::transform([](auto &&range) {
                    return std::stoll(std::string(range.begin(), range.end()));
                });
            for (const auto &unread_message_id : result) {
                conv->add_unread_message_ids(unread_message_id);
            }
        }

        lg::write(lg::level::info, "GetConversations({}) -> Ok({})", user_id,
                  rows.size());
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "GetConversations({}) -> Err({})",
                  request->user_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::DeleteConversation(grpc::ServerContext *context,
                                      const DeleteConversationRequest *request,
                                      DeleteConversationResponse *response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        db_->execute("UPDATE conversations SET is_deleted = 1 WHERE id = ?",
                     conversation_id);

        {
            link::GetTransactionsResponse response;
            auto *op = response.add_operations();
            op->set_type(link::OPERATION_TYPE_UPDATE);
            op->set_table_name("conversations");
            (*op->mutable_new_values())["is_deleted"] = "1";
            (*op->mutable_old_key_values())["id"] =
                std::to_string(conversation_id);
            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }

        lg::write(lg::level::info, "DeleteConversation({}) -> Ok()",
                  conversation_id);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
    lg:
        write(lg::level::error, "DeleteConversation({}) -> Err({})",
              request->conversation_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::SendMessage(grpc::ServerContext *context,
                               const SendMessageRequest *request,
                               SendMessageResponse *response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        sqlite3_int64 send_user_id = request->message_send_user_id();
        std::string content = request->message_content();

        if (content.empty()) {
            throw std::runtime_error("message content is empty");
        }

        db_->execute("BEGIN TRANSACTION");
        sqlite3_int64 message_id;
        {
            auto rows = db_->execute<sqlite3_int64>(
                "INSERT INTO messages (conversation_id, user_id, content) "
                "VALUES (?, ?, ?) RETURNING id",
                conversation_id, send_user_id, content);
            if (rows.empty()) {
                throw std::runtime_error("message not inserted");
            }
            message_id = std::get<0>(rows.at(0));
        }

        sqlite3_int64 recv_user_id;
        {
            auto rows = db_->execute<sqlite3_int64>(
                "SELECT user_id FROM conversations_users "
                "WHERE conversation_id = ? AND user_id != ?",
                conversation_id, send_user_id);
            if (rows.empty()) {
                throw std::runtime_error("failed to find user");
            }
            recv_user_id = std::get<0>(rows.at(0));
        }

        db_->execute("COMMIT");

        {
            link::GetTransactionsResponse response;
            auto *op = response.add_operations();
            op->set_type(link::OPERATION_TYPE_INSERT);
            op->set_table_name("messages");
            (*op->mutable_new_values())["id"] = std::to_string(message_id);
            (*op->mutable_new_values())["conversation_id"] =
                std::to_string(conversation_id);
            (*op->mutable_new_values())["user_id"] =
                std::to_string(send_user_id);
            (*op->mutable_new_values())["content"] = content;

            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }
        // attempts real-time streaming if we have a writer
        {
            std::shared_lock lock(user_id_to_writers_mutex_);

            // notifies the receiver
            if (auto it = user_id_to_writers_.find(recv_user_id);
                it != user_id_to_writers_.end()) {
                auto *writer = it->second.ReceiveMessage_writer;
                if (writer != nullptr) {
                    ReceiveMessageResponse item;
                    item.set_conversation_id(conversation_id);
                    Message message;
                    message.set_id(message_id);
                    message.set_send_user_id(send_user_id);
                    message.set_content(content);
                    item.set_allocated_message(&message);
                    writer->Write(item);
                    std::ignore = item.release_message();
                }
            }

            // notifies the sender
            if (auto it = user_id_to_writers_.find(send_user_id);
                it != user_id_to_writers_.end()) {
                auto *writer = it->second.ReceiveMessage_writer;
                if (writer != nullptr) {
                    ReceiveMessageResponse item;
                    item.set_conversation_id(conversation_id);
                    Message message;
                    message.set_id(message_id);
                    message.set_send_user_id(send_user_id);
                    message.set_content(content);
                    item.set_allocated_message(&message);
                    writer->Write(item);
                    std::ignore = item.release_message();
                }
            }
        }

        lg::write(lg::level::info, "SendMessage({},{},{}) -> Ok({})",
                  conversation_id, send_user_id, content, message_id);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        db_->execute("ROLLBACK");
        lg::write(lg::level::error, "SendMessage({},{},{}) -> Err({})",
                  request->conversation_id(), request->message_send_user_id(),
                  request->message_content(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::GetMessages(grpc::ServerContext *context,
                               const GetMessagesRequest *request,
                               GetMessagesResponse *response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        auto rows =
            db_->execute<sqlite3_int64, sqlite3_int64, bool, std::string>(
                "SELECT id, user_id, is_read, content "
                "FROM messages "
                "WHERE conversation_id = ? AND is_deleted = 0 "
                "ORDER BY created_at ASC",
                conversation_id);
        for (auto &[id, user_id, is_read, content] : rows) {
            auto *message = response->add_messages();
            message->set_id(id);
            message->set_send_user_id(user_id);
            message->set_content(content);
        }
        lg::write(lg::level::info, "GetMessages({}) -> Ok({})", conversation_id,
                  rows.size());
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "GetMessages({}) -> Err({})",
                  request->conversation_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::ReadMessages(grpc::ServerContext *context,
                                const ReadMessagesRequest *request,
                                ReadMessagesResponse *response) {
    try {
        std::string query =
            "UPDATE messages SET is_read = 1 WHERE user_id != ? AND id IN (";
        for (size_t i = 0; i < request->message_ids_size(); i++) {
            query += "?";
            if (i + 1 < request->message_ids_size()) {
                query += ", ";
            }
        }
        query += ") RETURNING user_id";
        lg::write(lg::level::trace, query);
        auto rows = db_->execute<sqlite3_int64>(query, request->user_id(),
                                                request->message_ids());
        if (rows.empty()) {
            throw std::runtime_error("failed to read messages");
        }
        auto &[recv_user_id] = rows.at(0);

        {
            link::GetTransactionsResponse response;
            for (const auto &message_id : request->message_ids()) {
                auto *op = response.add_operations();
                op->set_type(link::OPERATION_TYPE_UPDATE);
                op->set_table_name("messages");
                (*op->mutable_new_values())["is_read"] = "1";
                (*op->mutable_old_key_values())["id"] =
                    std::to_string(message_id);
            }
            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }

        {
            std::shared_lock lock(user_id_to_writers_mutex_);

            // notifies the sender
            if (auto it = user_id_to_writers_.find(request->user_id());
                it != user_id_to_writers_.end()) {
                auto *writer = it->second.ReceiveReadMessages_writer;
                if (writer != nullptr) {
                    ReceiveReadMessagesResponse item;
                    item.set_conversation_id(request->conversation_id());
                    for (const auto &message_id : request->message_ids()) {
                        item.add_message_ids(message_id);
                    }
                    writer->Write(item);
                }
            }

            // notifies the receiver
            if (auto it = user_id_to_writers_.find(recv_user_id);
                it != user_id_to_writers_.end()) {
                auto *writer = it->second.ReceiveReadMessages_writer;
                if (writer != nullptr) {
                    ReceiveReadMessagesResponse item;
                    item.set_conversation_id(request->conversation_id());
                    for (const auto &message_id : request->message_ids()) {
                        item.add_message_ids(message_id);
                    }
                    writer->Write(item);
                }
            }
        }

        lg::write(lg::level::info, "ReadMessages({},{}) -> Ok()",
                  request->user_id(), request->message_ids_size());
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "ReadMessages({},{}) -> Err({})",
                  request->user_id(), request->message_ids_size(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::DeleteMessage(grpc::ServerContext *context,
                                 const DeleteMessageRequest *request,
                                 DeleteMessageResponse *response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        sqlite3_int64 message_id = request->message_id();
        db_->execute(
            "UPDATE messages SET is_deleted = 1 "
            "WHERE conversation_id = ? AND id = ?",
            conversation_id, message_id);

        {
            link::GetTransactionsResponse response;
            auto *op = response.add_operations();
            op->set_type(link::OPERATION_TYPE_UPDATE);
            op->set_table_name("messages");
            (*op->mutable_new_values())["is_deleted"] = "1";
            (*op->mutable_old_key_values())["id"] = std::to_string(message_id);

            std::shared_lock lock(cluster_writers_mutex_);
            for (auto *writer : cluster_writers_) {
                writer->Write(response);
            }
        }

        lg::write(lg::level::info, "DeleteMessage({},{}) -> Ok()",
                  conversation_id, message_id);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        lg::write(lg::level::error, "DeleteMessage({},{}) -> Err({})",
                  request->conversation_id(), request->message_id(), e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Impl::ReceiveMessage(
    grpc::ServerContext *context, const ReceiveMessageRequest *request,
    grpc::ServerWriter<ReceiveMessageResponse> *writer) {
    lg::write(lg::level::info, "ReceiveMessageRequest({})", request->user_id());
    std::unique_lock lock(user_id_to_writers_mutex_);
    if (auto [it, inserted] = user_id_to_writers_.try_emplace(
            request->user_id(), Writers{.ReceiveMessage_writer = writer,
                                        .ReceiveReadMessages_writer = nullptr});
        !inserted) {
        it->second.ReceiveMessage_writer = writer;
    }
    lg::write(lg::level::info, "ReceiveMessageRequest({}) -> +",
              request->user_id());
    lock.unlock();

    // Keep the stream open until client cancels or server stops
    while (!context->IsCancelled()) {
        // This spin is naive (real code might do condition_variable wait, etc.)
    }

    lock.lock();
    if (auto it = user_id_to_writers_.find(request->user_id());
        it != user_id_to_writers_.end()) {
        if (it->second.ReceiveReadMessages_writer == nullptr) {
            user_id_to_writers_.erase(it);
        } else {
            it->second.ReceiveMessage_writer = nullptr;
        }
    }
    lock.unlock();
    lg::write(lg::level::info, "ReceiveMessageRequest({}) -> -",
              request->user_id());
    return grpc::Status::CANCELLED;
}

grpc::Status Impl::ReceiveReadMessages(
    grpc::ServerContext *context, const ReceiveReadMessagesRequest *request,
    grpc::ServerWriter<ReceiveReadMessagesResponse> *writer) {
    lg::write(lg::level::info, "ReceiveReadMessages({})", request->user_id());
    std::unique_lock lock(user_id_to_writers_mutex_);
    if (auto [it, inserted] = user_id_to_writers_.try_emplace(
            request->user_id(), Writers{.ReceiveMessage_writer = nullptr,
                                        .ReceiveReadMessages_writer = writer});
        !inserted) {
        it->second.ReceiveReadMessages_writer = writer;
    }
    lg::write(lg::level::info, "ReceiveReadMessages({}) -> +",
              request->user_id());
    lock.unlock();

    // Keep the stream open until client cancels or server stops
    while (!context->IsCancelled()) {
        // This spin is naive (real code might do condition_variable wait, etc.)
    }

    lock.lock();
    if (auto it = user_id_to_writers_.find(request->user_id());
        it != user_id_to_writers_.end()) {
        if (it->second.ReceiveMessage_writer == nullptr) {
            user_id_to_writers_.erase(it);
        } else {
            it->second.ReceiveReadMessages_writer = nullptr;
        }
    }
    lock.unlock();
    lg::write(lg::level::info, "ReceiveReadMessages({}) -> -",
              request->user_id());
    return grpc::Status::CANCELLED;
}

}  // namespace main
}  // namespace service
}  // namespace converse
