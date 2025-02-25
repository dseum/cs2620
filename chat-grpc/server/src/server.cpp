#include "server.hpp"

#include <argon2.h>
#include <random>
#include <stdexcept>
#include <format>
#include <iostream>
#include <cstring>   // for strdup, strcmp

bool verify_password(const std::string& password,
                     const std::string& password_encoded) {
    uint8_t* pwd = (uint8_t*)strdup(password.c_str());
    uint32_t pwdlen = static_cast<uint32_t>(strlen((char*)pwd));

    int rc = argon2i_verify(password_encoded.c_str(), pwd, pwdlen);
    free(pwd);
    return rc == ARGON2_OK;
}

std::string generate_password_encoded(const std::string& password,
                                      std::array<uint8_t, SALTLEN> salt) {
    if (salt == std::array<uint8_t, SALTLEN>{0}) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        for (size_t i = 0; i < SALTLEN; i++) {
            salt[i] = dis(gen);
        }
    }
    uint8_t* pwd = (uint8_t*)strdup(password.c_str());
    uint32_t pwdlen = static_cast<uint32_t>(strlen((char*)pwd));

    uint32_t t_cost = 2;
    uint32_t m_cost = (1 << 16);
    uint32_t parallelism = 1;

    size_t encodedlen = argon2_encodedlen(t_cost, m_cost, parallelism, SALTLEN,
                                          HASHLEN, Argon2_i);

    std::string encoded(encodedlen, '\0');
    int rc = argon2i_hash_encoded(t_cost, m_cost, parallelism,
                                  pwd, pwdlen,
                                  salt.data(), SALTLEN,
                                  HASHLEN,
                                  encoded.data(), encodedlen);

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

MyConverseServiceImpl::MyConverseServiceImpl() {
    db_ = std::make_unique<Db>(); 
}

MyConverseServiceImpl::~MyConverseServiceImpl() = default;

::grpc::Status MyConverseServiceImpl::SignupUser(
    ::grpc::ServerContext* context,
    const converse::SignupUserRequest* request,
    converse::SignupUserResponse* response) {
    try {
        // 1) Insert user (no RETURNING)
        std::string password_encoded =
            generate_password_encoded(request->password());

        db_->execute(
            "INSERT INTO users (username, password_encoded) VALUES (?, ?)",
            request->username(), password_encoded);

        // 2) Retrieve auto-increment ID
        auto rows = db_->execute<sqlite3_int64>("SELECT last_insert_rowid()");
        if (rows.empty()) {
            throw std::runtime_error("failed to create user (no rowid)");
        }
        auto [user_id] = rows.at(0);

        response->set_user_id(user_id);
        std::cout << "Processed SignupUser for username: "
                  << request->username() << " -> new ID " << user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::SigninUser(
    ::grpc::ServerContext* context,
    const converse::SigninUserRequest* request,
    converse::SigninUserResponse* response) {
    try {
        auto rows = db_->execute<sqlite3_int64, std::string>(
            "SELECT id, password_encoded FROM users "
            "WHERE username = ? AND is_deleted = 0",
            request->username());
        if (rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [id, password_encoded] = rows.at(0);
        if (!verify_password(request->password(), password_encoded)) {
            throw std::runtime_error("incorrect password");
        }
        response->set_user_id(id);
        std::cout << "Processed SigninUser for username: "
                  << request->username() << " (ID=" << id << ")\n";
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::SignoutUser(
    ::grpc::ServerContext* context,
    const converse::SignoutUserRequest* request,
    converse::SignoutUserResponse* response) {
    // Not implemented in this snippet
    return ::grpc::Status::OK;
}

::grpc::Status MyConverseServiceImpl::DeleteUser(
    ::grpc::ServerContext* context,
    const converse::DeleteUserRequest* request,
    converse::DeleteUserResponse* response) {
    try {
        db_->execute("BEGIN TRANSACTION");
        sqlite3_int64 user_id = request->user_id();
        auto rows = db_->execute<std::string>(
            "SELECT password_encoded FROM users WHERE id = ? AND is_deleted = 0",
            user_id);
        if (rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [password_encoded] = rows.at(0);
        if (!verify_password(request->password(), password_encoded)) {
            throw std::runtime_error("incorrect password");
        }
        db_->execute("UPDATE users SET is_deleted = 1 WHERE id = ?", user_id);
        db_->execute("COMMIT");
        std::cout << "Processed DeleteUser for user ID: " << user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        db_->execute("ROLLBACK");
        std::cerr << "Error in DeleteUser: " << e.what() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::GetOtherUsers(
    ::grpc::ServerContext* context,
    const converse::GetOtherUsersRequest* request,
    converse::GetOtherUsersResponse* response) {
    try {
        sqlite3_int64 user_id = request->user_id();
        std::string like_pattern = "%" + request->query() + "%";
        auto rows = db_->execute<sqlite3_int64, std::string>(
            "SELECT id, username FROM users "
            "WHERE id != ? AND is_deleted = 0 AND username LIKE ? "
            "LIMIT ? OFFSET ?",
            user_id,
            like_pattern,
            static_cast<sqlite3_int64>(request->limit()),
            static_cast<sqlite3_int64>(request->offset()));
        for (const auto& [id, username] : rows) {
            auto* user = response->add_users();
            user->set_user_id(id);
            user->set_username(username);
        }
        std::cout << "Processed GetOtherUsers for user ID: "
                  << user_id << ", found " << rows.size() << " users." << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::CreateConversation(
    ::grpc::ServerContext* context,
    const converse::CreateConversationRequest* request,
    converse::CreateConversationResponse* response) {
    try {
        sqlite3_int64 send_user_id = request->user_id();
        sqlite3_int64 recv_user_id = request->other_user_id();
        db_->execute("BEGIN TRANSACTION");

        // 1) Insert new row in `conversations` table
        db_->execute("INSERT INTO conversations DEFAULT VALUES");

        // 2) get last_insert_rowid() for conversation_id
        auto rows_conv = db_->execute<sqlite3_int64>(
            "SELECT last_insert_rowid()");
        if (rows_conv.empty()) {
            throw std::runtime_error("failed to create conversation");
        }
        auto [conversation_id] = rows_conv.at(0);

        // 3) Insert records in `conversations_users`
        db_->execute(
            "INSERT INTO conversations_users (conversation_id, user_id) "
            "VALUES (?, ?), (?, ?)",
            conversation_id, send_user_id,
            conversation_id, recv_user_id);

        // 4) check that receiving user actually exists
        auto recv_user_rows = db_->execute<std::string>(
            "SELECT username FROM users WHERE id = ? AND is_deleted = 0",
            recv_user_id);
        if (recv_user_rows.empty()) {
            throw std::runtime_error("failed to find user");
        }

        auto [recv_user_username] = recv_user_rows.at(0);

        db_->execute("COMMIT");

        // 5) fill response
        response->set_conversation_id(conversation_id);
        response->set_recv_user_id(recv_user_id);
        response->set_recv_username(recv_user_username);

        std::cout << "Processed CreateConversation for sender ID: "
                  << send_user_id << " and receiver ID: "
                  << recv_user_id
                  << " -> conversation_id " << conversation_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        db_->execute("ROLLBACK");
        std::cerr << "Error in CreateConversation: " << e.what() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::GetConversations(
    ::grpc::ServerContext* context,
    const converse::GetConversationsRequest* request,
    converse::GetConversationsResponse* response) {
    try {
        sqlite3_int64 user_id = request->user_id();
        auto rows = db_->execute<sqlite3_int64, sqlite3_int64, std::string, sqlite3_int64>(
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
                   COALESCE(COUNT(m.id), 0) AS unread_messages
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

        std::cout << "Found " << rows.size()
                  << " conversation(s) for user " << user_id << std::endl;

        for (auto& row : rows) {
            auto* conv = response->add_conversations();
            conv->set_conversation_id(std::get<0>(row));
            conv->set_recv_user_id(std::get<1>(row));
            conv->set_recv_username(std::get<2>(row));
            conv->set_unread_count(std::get<3>(row));
        }
        std::cout << "Processed GetConversations for user ID: "
                  << user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::DeleteConversation(
    ::grpc::ServerContext* context,
    const converse::DeleteConversationRequest* request,
    converse::DeleteConversationResponse* response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        db_->execute("UPDATE conversations SET is_deleted = 1 WHERE id = ?",
                     conversation_id);
        std::cout << "Processed DeleteConversation for conversation ID: "
                  << conversation_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::SendMessage(
    ::grpc::ServerContext* context,
    const converse::SendMessageRequest* request,
    converse::SendMessageResponse* response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        sqlite3_int64 send_user_id = request->send_user_id();
        std::string content = request->content();

        db_->execute("BEGIN TRANSACTION");
        // 1) Insert new message
        db_->execute(
            "INSERT INTO messages (conversation_id, user_id, content) "
            "VALUES (?, ?, ?)",
            conversation_id, send_user_id, content);

        // 2) get last_insert_rowid() for message
        auto rows = db_->execute<sqlite3_int64>(
            "SELECT last_insert_rowid()");
        if (rows.empty()) {
            throw std::runtime_error("failed to create message");
        }
        auto [message_id] = rows.at(0);

        // 3) find receiving user in that conversation
        auto recv_rows = db_->execute<sqlite3_int64>(
            "SELECT user_id FROM conversations_users "
            "WHERE conversation_id = ? AND user_id != ?",
            conversation_id, send_user_id);
        if (recv_rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [recv_user_id] = recv_rows.at(0);

        // 4) attempt real-time streaming if we have a writer
        {
            std::shared_lock lock(user_id_to_conversation_ids_mutex_);
            // notify the receiver
            if (auto it = user_id_to_writer_.find(recv_user_id);
                it != user_id_to_writer_.end()) {
                auto* writer = it->second;
                converse::MessageWithConversationId item;
                item.set_conversation_id(conversation_id);
                converse::Message message;
                message.set_message_id(message_id);
                message.set_send_user_id(send_user_id);
                message.set_is_read(false);
                message.set_content(content);
                item.set_allocated_message(&message);
                writer->Write(item);
                (void)item.release_message();
            }
            // notify the sender
            if (auto it = user_id_to_writer_.find(send_user_id);
                it != user_id_to_writer_.end()) {
                auto* writer = it->second;
                converse::MessageWithConversationId item;
                item.set_conversation_id(conversation_id);
                converse::Message message;
                message.set_message_id(message_id);
                message.set_send_user_id(send_user_id);
                message.set_is_read(true);  // local user sees it as read
                message.set_content(content);
                item.set_allocated_message(&message);
                writer->Write(item);
                (void)item.release_message();
            }
        }

        db_->execute("COMMIT");
        std::cout << "Processed SendMessage for conversation ID: "
                  << conversation_id << " and sender ID: "
                  << send_user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        db_->execute("ROLLBACK");
        std::cerr << "Error in SendMessage: " << e.what() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::GetMessages(
    ::grpc::ServerContext* context,
    const converse::GetMessagesRequest* request,
    converse::GetMessagesResponse* response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        std::cout << "conversation_id: " << conversation_id << std::endl;
        auto rows = db_->execute<sqlite3_int64, sqlite3_int64, bool, std::string>(
            "SELECT id, user_id, is_read, content "
            "FROM messages "
            "WHERE conversation_id = ? AND is_deleted = 0 "
            "ORDER BY created_at ASC",
            conversation_id);
        for (auto& [id, user_id, is_read, content] : rows) {
            auto* message = response->add_messages();
            message->set_message_id(id);
            message->set_send_user_id(user_id);
            message->set_is_read(is_read);
            message->set_content(content);
        }
        std::cout << "Processed GetMessages for conversation ID: "
                  << conversation_id << ", found " << rows.size() << " messages.\n";
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::DeleteMessage(
    ::grpc::ServerContext* context,
    const converse::DeleteMessageRequest* request,
    converse::DeleteMessageResponse* response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        sqlite3_int64 message_id = request->message_id();
        db_->execute(
            "UPDATE messages SET is_deleted = 1 "
            "WHERE conversation_id = ? AND id = ?",
            conversation_id, message_id);
        std::cout << "Processed DeleteMessage for message ID: "
                  << message_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::ReceiveMessage(
    ::grpc::ServerContext* context,
    const converse::ReceiveMessageRequest* request,
    ::grpc::ServerWriter<converse::MessageWithConversationId>* writer) {
    std::cout << "Received stream for user ID: "
              << request->user_id() << std::endl;
    std::unique_lock lock(user_id_to_conversation_ids_mutex_);
    user_id_to_writer_.emplace(request->user_id(), writer);
    std::cout << "Added stream for user ID: "
              << request->user_id() << std::endl;
    lock.unlock();

    // Keep the stream open until client cancels or server stops
    while (!context->IsCancelled()) {
        // This spin is naive (real code might do condition_variable wait, etc.)
    }

    lock.lock();
    std::cout << "Removed stream for user ID: "
              << request->user_id() << std::endl;
    user_id_to_writer_.erase(request->user_id());
    lock.unlock();

    return ::grpc::Status::CANCELLED;
}
