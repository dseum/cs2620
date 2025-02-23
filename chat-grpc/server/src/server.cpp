#include "server.hpp"
#include "operations.hpp"
#include <iostream>

MyConverseServiceImpl::MyConverseServiceImpl() {
    db_ = std::make_unique<Db>();
}

MyConverseServiceImpl::~MyConverseServiceImpl() = default;

::grpc::Status MyConverseServiceImpl::SignupUser(::grpc::ServerContext* context,
                                                 const converse::SignupUserRequest* request,
                                                 converse::SignupUserResponse* response) {
    try {
        std::string password_encoded = generate_password_encoded(request->password());
        auto rows = db_->execute<sqlite3_int64>(
            "INSERT INTO users (username, password_encoded) VALUES (?, ?) RETURNING id",
            request->username(), password_encoded);
        if (rows.empty()) {
            throw std::runtime_error("failed to create user");
        }
        auto [user_id] = rows.at(0);
        response->set_user_id(user_id);
        std::cout << "Processed SignupUser for username: " << request->username() << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}


::grpc::Status MyConverseServiceImpl::SigninUser(::grpc::ServerContext* context,
                                                 const converse::SigninUserRequest* request,
                                                 converse::SigninUserResponse* response) {
    try {
        auto rows = db_->execute<sqlite3_int64, std::string>(
            "SELECT id, password_encoded FROM users WHERE username = ? AND is_deleted = 0",
            request->username());
        if (rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [id, password_encoded] = rows.at(0);
        if (!verify_password(request->password(), password_encoded)) {
            throw std::runtime_error("incorrect password");
        }
        response->set_user_id(id);
        std::cout << "Processed SigninUser for username: " << request->username() << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::SignoutUser(::grpc::ServerContext* context,
                                                  const converse::SignoutUserRequest* request,
                                                  converse::SignoutUserResponse* response) {
    return ::grpc::Status::OK; 
}

::grpc::Status MyConverseServiceImpl::DeleteUser(::grpc::ServerContext* context,
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

::grpc::Status MyConverseServiceImpl::GetOtherUsers(::grpc::ServerContext* context,
                                                    const converse::GetOtherUsersRequest* request,
                                                    converse::GetOtherUsersResponse* response) {
    try {
        sqlite3_int64 user_id = request->user_id();
        std::string like_pattern = "%" + request->query() + "%";
        auto rows = db_->execute<sqlite3_int64, std::string>(
            "SELECT id, username FROM users WHERE id != ? AND is_deleted = 0 AND username LIKE ? LIMIT ? OFFSET ?",
            user_id, like_pattern, request->limit(), request->offset());
        for (const auto& [id, username] : rows) {
            auto* user = response->add_users();
            user->set_user_id(id);
            user->set_username(username);
        }
        std::cout << "Processed GetOtherUsers for user ID: " << user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::CreateConversation(::grpc::ServerContext* context,
                                                         const converse::CreateConversationRequest* request,
                                                         converse::CreateConversationResponse* response) {
    try {
        sqlite3_int64 send_user_id = request->send_user_id();
        sqlite3_int64 recv_user_id = request->recv_user_id();
        db_->execute("BEGIN TRANSACTION");
        auto rows = db_->execute<sqlite3_int64>(
            "INSERT INTO conversations DEFAULT VALUES RETURNING id");
        if (rows.empty()) {
            throw std::runtime_error("failed to create conversation");
        }
        auto [conversation_id] = rows.at(0);
        db_->execute(
            "INSERT INTO conversations_users (conversation_id, user_id) VALUES (?, ?), (?, ?)",
            conversation_id, send_user_id, conversation_id, recv_user_id);
        auto recv_user_rows = db_->execute<std::string>(
            "SELECT username FROM users WHERE id = ?", recv_user_id);
        if (recv_user_rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [recv_user_username] = recv_user_rows.at(0);
        db_->execute("COMMIT");
        response->set_conversation_id(conversation_id);
        response->set_recv_user_username(recv_user_username);
        std::cout << "Processed CreateConversation for sender ID: " << send_user_id
                  << " and receiver ID: " << recv_user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        db_->execute("ROLLBACK");
        std::cerr << "Error in CreateConversation: " << e.what() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::GetConversation(::grpc::ServerContext* context,
                                                      const converse::GetConversationRequest* request,
                                                      converse::GetConversationResponse* response) {
    try{
        sqlite3_int64 user_id = request->user_id();
        auto rows = db_->execute<sqlite3_int64, std::string>(
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
        std::cout << "rows: " << rows.size() << std::endl;
        for (const auto& row : rows) {
            auto* conv = response->add_conversations();
            conv->set_conversation_id(std::get<0>(row));
            conv->set_other_user_id(std::get<1>(row));
            conv->set_other_user_username(std::get<2>(row));
            conv->set_unread_messages(std::get<3>(row));
        }
        std::cout << "Processed GetConversation for user ID: " << user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::DeleteConversation(::grpc::ServerContext* context,
                                                         const converse::DeleteConversationRequest* request,
                                                         converse::DeleteConversationResponse* response) {
    try{
        sqlite3_int64 conversation_id = request->conversation_id();
        db_->execute("UPDATE conversations SET is_deleted = 1 WHERE id = ?", conversation_id);
        std::cout << "Processed DeleteConversation for conversation ID: " << conversation_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::SendMessage(::grpc::ServerContext* context,
                                                  const converse::SendMessageRequest* request,
                                                  converse::SendMessageResponse* response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        sqlite3_int64 send_user_id = request->send_user_id();
        std::string content = request->content();
        db_->execute("BEGIN TRANSACTION");
        auto rows = db_->execute<sqlite3_int64>(
            "INSERT INTO messages (conversation_id, user_id, content) VALUES (?, ?, ?) RETURNING id",
            conversation_id, send_user_id, content);
        if (rows.empty()) {
            throw std::runtime_error("failed to create message");
        }
        auto [message_id] = rows.at(0);
        rows = db_->execute<sqlite3_int64>(
            "SELECT user_id FROM conversations_users WHERE conversation_id = ? AND user_id != ?",
            conversation_id, send_user_id);
        if (rows.empty()) {
            throw std::runtime_error("failed to find user");
        }
        auto [recv_user_id] = rows.at(0);
        response->set_message_id(message_id);
        response->set_recv_user_id(recv_user_id);
        response->set_content(content);
        db_->execute("COMMIT");
        std::cout << "Processed SendMessage for conversation ID: " << conversation_id
                  << " and sender ID: " << send_user_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        db_->execute("ROLLBACK");
        std::cerr << "Error in SendMessage: " << e.what() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::GetMessages(::grpc::ServerContext* context,
                                                  const converse::GetMessagesRequest* request,
                                                  converse::GetMessagesResponse* response) {
    try {
        sqlite3_int64 conversation_id = request->conversation_id();
        std::cout << "conversation_id: " << conversation_id << std::endl;
        auto rows = db_->execute<sqlite3_int64, sqlite3_int64, bool, std::string>(
            "SELECT id, user_id, is_read, content FROM messages "
            "WHERE conversation_id = ? AND is_deleted = 0 ORDER BY "
            "created_at ASC",
            conversation_id);
        for (const auto& [id, user_id, is_read, content] : rows) {
            auto* message = response->add_messages();
            message->set_message_id(id);
            message->set_user_id(user_id);
            message->set_is_read(is_read);
            message->set_content(content);
        }
        std::cout << "Processed GetMessages for conversation ID: " << conversation_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}

::grpc::Status MyConverseServiceImpl::DeleteMessage(::grpc::ServerContext* context,
                                                    const converse::DeleteMessageRequest* request,
                                                    converse::DeleteMessageResponse* response) {
    try{
        sqlite3_int64 conversation_id = request->conversation_id();
        sqlite3_int64 message_id = request->message_id();
        db_->execute("UPDATE messages SET is_deleted = 1 WHERE conversation_id "
            "= ? AND id = ?",
            conversation_id, message_id);
        std::cout << "Processed DeleteMessage for message ID: " << message_id << std::endl;
        return ::grpc::Status::OK;
    } catch (const std::exception& e) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
    }
}
