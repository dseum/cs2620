#include "backend.hpp"

#include <grpcpp/client_context.h>
#include <grpcpp/grpcpp.h>

#include <converse/logging/core.hpp>
#include <format>

#include "reader.hpp"

namespace lg = converse::logging;

namespace converse {
namespace qobject {

User::User(int id, const QString &username, QObject *parent)
    : QObject(parent), id_(id), username_(username) {}

int User::id() const { return id_; }

QString User::username() const { return username_; }

Conversation::Conversation(int id, int recv_user_id,
                           const QString &recv_user_username, int unread_count,
                           QObject *parent)
    : QObject(parent),
      id_(id),
      recv_user_id_(recv_user_id),
      recv_user_username_(recv_user_username),
      unread_count_(unread_count) {}

int Conversation::id() const { return id_; }

int Conversation::recv_user_id() const { return recv_user_id_; }

QString Conversation::recv_user_username() const { return recv_user_username_; }

int Conversation::unread_count() const { return unread_count_; }

void Conversation::set_unread_count(int value) { unread_count_ = value; }

Message::Message(int id, int send_user_id, bool is_read, const QString &content,
                 QObject *parent)
    : QObject(parent),
      id_(id),
      send_user_id_(send_user_id),
      is_read_(is_read),
      content_(content) {}

int Message::id() const { return id_; }

int Message::send_user_id() const { return send_user_id_; }

bool Message::is_read() const { return is_read_; }

QString Message::content() const { return content_; }

ConversationsModel::ConversationsModel(QObject *parent)
    : QAbstractListModel(parent) {}

int ConversationsModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return conversations_.size();
}

QVariant ConversationsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= conversations_.size())
        return QVariant();

    Conversation *conversation = conversations_.at(index.row());
    switch (role) {
        case IdRole:
            return conversation->id();
        case RecvUserIdRole:
            return conversation->recv_user_id();
        case RecvUserUsernameRole:
            return conversation->recv_user_username();
        case UnreadCountRole:
            return conversation->unread_count();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ConversationsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[RecvUserIdRole] = "recvUserId";
    roles[RecvUserUsernameRole] = "recvUserUsername";
    roles[UnreadCountRole] = "unreadCount";
    return roles;
}

QVariantMap ConversationsModel::get(int index) {
    QVariantMap conversation;
    if (index < 0 || index >= conversations_.size()) return conversation;

    Conversation *c = conversations_.at(index);
    conversation.insert("id", c->id());
    conversation.insert("recvUserId", c->recv_user_id());
    conversation.insert("recvUserUsername", c->recv_user_username());
    conversation.insert("unreadCount", c->unread_count());
    return conversation;
}

std::pair<Conversation *, int> ConversationsModel::getInstanceById(
    int conversation_id) {
    auto it = std::find_if(conversations_.begin(), conversations_.end(),
                           [conversation_id](Conversation *conversation) {
                               return conversation->id() == conversation_id;
                           });
    if (it == conversations_.end()) {
        return {nullptr, -1};
    }
    int index = std::distance(conversations_.begin(), it);
    return {*it, index};
}

void ConversationsModel::remove(int index) {
    if (index < 0 || index >= conversations_.size()) return;

    beginRemoveRows(QModelIndex(), index, index);
    Conversation *conversation = conversations_.takeAt(index);
    delete conversation;
    endRemoveRows();
}

void ConversationsModel::removeById(int conversation_id) {
    auto it = std::find_if(conversations_.begin(), conversations_.end(),
                           [conversation_id](Conversation *conversation) {
                               return conversation->id() == conversation_id;
                           });
    if (it != conversations_.end()) {
        remove(std::distance(conversations_.begin(), it));
    }
}

void ConversationsModel::prepend(Conversation *conversation) {
    beginInsertRows(QModelIndex(), 0, 0);
    conversations_.prepend(conversation);
    endInsertRows();
}

void ConversationsModel::append(Conversation *conversation) {
    beginInsertRows(QModelIndex(), conversations_.size(),
                    conversations_.size());
    conversations_.append(conversation);
    endInsertRows();
}

void ConversationsModel::set(const QList<Conversation *> &conversations) {
    beginResetModel();
    qDeleteAll(conversations_);
    conversations_ = conversations;
    endResetModel();
}

void ConversationsModel::move(int from, int to) {
    if (from < 0 || from >= conversations_.size() || to < 0 ||
        to >= conversations_.size()) {
        return;
    }
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    conversations_.move(from, to);
    endMoveRows();
}

MessagesModel::MessagesModel(QObject *parent) : QAbstractListModel(parent) {}

int MessagesModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return messages_.size();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= messages_.size())
        return QVariant();

    Message *message = messages_.at(index.row());
    switch (role) {
        case IdRole:
            return message->id();
        case SendUserIdRole:
            return message->send_user_id();
        case IsReadRole:
            return message->is_read();
        case ContentRole:
            return message->content();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> MessagesModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[SendUserIdRole] = "sendUserId";
    roles[IsReadRole] = "isRead";
    roles[ContentRole] = "content";
    return roles;
}

std::optional<int> MessagesModel::conversation_id() const {
    return conversation_id_;
}

QVariantMap MessagesModel::get(int index) {
    QVariantMap message;
    if (index < 0 || index >= messages_.size()) return message;

    Message *m = messages_.at(index);
    message.insert("id", m->id());
    message.insert("sendUserId", m->send_user_id());
    message.insert("isRead", m->is_read());
    message.insert("content", m->content());
    return message;
}

Message *MessagesModel::getInstanceById(int message_id) {
    auto it = std::find_if(
        messages_.begin(), messages_.end(),
        [message_id](Message *message) { return message->id() == message_id; });
    if (it == messages_.end()) {
        return nullptr;
    }
    return *it;
}

void MessagesModel::remove(int index) {
    if (index < 0 || index >= messages_.size()) return;

    beginRemoveRows(QModelIndex(), index, index);
    Message *message = messages_.takeAt(index);
    delete message;
    endRemoveRows();
}

void MessagesModel::removeById(int message_id) {
    auto it = std::find_if(
        messages_.begin(), messages_.end(),
        [message_id](Message *message) { return message->id() == message_id; });
    if (it != messages_.end()) {
        remove(std::distance(messages_.begin(), it));
    }
}

void MessagesModel::append(Message *message) {
    beginInsertRows(QModelIndex(), messages_.size(), messages_.size());
    messages_.append(message);
    endInsertRows();
    emit messageAppended();
}

void MessagesModel::set(int conversation_id, const QList<Message *> &messages) {
    beginResetModel();
    qDeleteAll(messages_);
    conversation_id_ = conversation_id;
    messages_ = messages;
    endResetModel();
}

void MessagesModel::reset() {
    beginResetModel();
    qDeleteAll(messages_);
    conversation_id_.reset();
    endResetModel();
}

OtherUsersModel::OtherUsersModel(QObject *parent)
    : QAbstractListModel(parent) {}

int OtherUsersModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return other_users_.size();
}

QVariant OtherUsersModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= other_users_.size())
        return QVariant();

    User *user = other_users_.at(index.row());
    switch (role) {
        case IdRole:
            return user->id();
        case UsernameRole:
            return user->username();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> OtherUsersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[UsernameRole] = "username";
    return roles;
}

QVariantMap OtherUsersModel::get(int index) {
    QVariantMap user;
    if (index < 0 || index >= other_users_.size()) return user;

    User *u = other_users_.at(index);
    user.insert("id", u->id());
    user.insert("username", u->username());
    return user;
}

void OtherUsersModel::append(User *user) {
    beginInsertRows(QModelIndex(), other_users_.size(), other_users_.size());
    other_users_.append(user);
    endInsertRows();
}

void OtherUsersModel::set(const QList<User *> &other_users) {
    beginResetModel();
    other_users_ = other_users;
    endResetModel();
}

Backend::Backend(const QString &host, int port, QObject *parent)
    : QObject(parent), user_(nullptr), conversation_(nullptr) {
    std::string address(std::format("{}:{}", host.toStdString(), port));
    channel_ = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    try {
        stub_ = service::main::MainService::NewStub(channel_);
        lg::write(lg::level::info, "connected to {}", address);
    } catch (std::exception &e) {
        lg::write(lg::level::error, "{}", e.what());
    }
}

User *Backend::user() const { return user_; }

void Backend::set_user(User *value) {
    if (user_ != value) {
        if (user_) {
            ReceiveMessage_reader_->TryCancel();
            delete user_;
            ReceiveMessage_reader_->Await();
        }
        user_ = value;
        emit userChanged();
        if (user_) {
            ReceiveMessage_request_.set_user_id(user_->id());
            auto ReceiveMessage_reader =
                new service::main::reader::ReceiveMessageReader(
                    stub_.get(), ReceiveMessage_request_,
                    [&](const grpc::Status &status,
                        const service::main::ReceiveMessageResponse &response) {
                        if (status.ok()) {
                            lg::write(lg::level::info,
                                      "ReceiveMessage() -> Ok({},{})",
                                      response.conversation_id(),
                                      response.message().id());
                            emit receiveMessageResponse(
                                response.conversation_id(),
                                new Message(response.message().id(),
                                            response.message().send_user_id(),
                                            false,
                                            QString::fromStdString(
                                                response.message().content())));
                        } else {
                            lg::write(lg::level::error,
                                      "ReceiveMessage() -> Err({})",
                                      status.error_message());
                        }
                    });
            ReceiveMessage_reader_.reset(ReceiveMessage_reader);
        } else {
            ReceiveMessage_reader_.reset();
        }
    }
}

Conversation *Backend::conversation() const { return conversation_; }

void Backend::set_conversation(Conversation *value) {
    if (conversation_ != value) {
        if (conversation_) {
            delete conversation_;
        }
        conversation_ = value;
        emit conversationChanged();
    }
}

void Backend::setUser(int id, const QString &username) {
    set_user(new User(id, username));
}

void Backend::setConversation(int id, int recv_user_id,
                              const QString &recv_user_username,
                              int unread_count) {
    set_conversation(
        new Conversation(id, recv_user_id, recv_user_username, unread_count));
}

void Backend::requestSigninUser(const QString &user_username,
                                const QString &user_password) {
    grpc::ClientContext context;
    service::main::SigninUserRequest request;
    request.set_user_username(user_username.toStdString());
    request.set_user_password(user_password.toStdString());
    service::main::SigninUserResponse response;
    grpc::Status status = stub_->SigninUser(&context, request, &response);
    if (status.ok()) {
        lg::write(lg::level::info, "SigninUser({}) -> Ok({})",
                  user_username.toStdString(), response.user_id());
        set_user(new User(response.user_id(), user_username));
    } else {
        lg::write(lg::level::error, "SigninUser({}) -> Err({})",
                  user_username.toStdString(), status.error_message());
    }
    emit signinUserResponse(status.ok());
}

void Backend::requestSignupUser(const QString &user_username,
                                const QString &user_password) {
    grpc::ClientContext context;
    service::main::SignupUserRequest request;
    request.set_user_username(user_username.toStdString());
    request.set_user_password(user_password.toStdString());
    service::main::SignupUserResponse response;
    grpc::Status status = stub_->SignupUser(&context, request, &response);
    if (status.ok()) {
        lg::write(lg::level::info, "SignupUser({}) -> Ok({})",
                  user_username.toStdString(), response.user_id());
        set_user(new User(response.user_id(), user_username));
    } else {
        lg::write(lg::level::error, "SignupUser({}) -> Err({})",
                  user_username.toStdString(), status.error_message());
    }
    emit signupUserResponse(status.ok());
}

void Backend::requestSignoutUser() {
    grpc::ClientContext context;
    service::main::SignoutUserRequest request;
    request.set_user_id(user_->id());
    service::main::SignoutUserResponse response;
    grpc::Status status = stub_->SignoutUser(&context, request, &response);
    if (status.ok()) {
        lg::write(lg::level::info, "SignoutUser({}) -> Ok()", user_->id());
        set_user(nullptr);
    } else {
        lg::write(lg::level::error, "SignoutUser({}) -> Err({})", user_->id(),
                  status.error_message());
    }
    emit signoutUserResponse(status.ok());
}

void Backend::requestDeleteUser(const QString &user_password) {
    grpc::ClientContext context;
    service::main::DeleteUserRequest request;
    request.set_user_id(user_->id());
    request.set_user_password(user_password.toStdString());
    service::main::DeleteUserResponse response;
    grpc::Status status = stub_->DeleteUser(&context, request, &response);
    if (status.ok()) {
        lg::write(lg::level::info, "DeleteUser({}) -> Ok()", user_->id());
        set_user(nullptr);
    } else {
        lg::write(lg::level::error, "DeleteUser({}) -> Err({})", user_->id(),
                  status.error_message());
    }
    emit deleteUserResponse(status.ok());
}

void Backend::requestGetOtherUsers(const QString &query) {
    grpc::ClientContext context;
    service::main::GetOtherUsersRequest request;
    request.set_user_id(user_->id());
    request.set_query(query.toStdString());
    request.set_limit(10);
    service::main::GetOtherUsersResponse response;
    grpc::Status status = stub_->GetOtherUsers(&context, request, &response);
    QList<User *> other_users;
    if (status.ok()) {
        lg::write(lg::level::info, "GetOtherUsers({}) -> Ok({})",
                  query.toStdString(), response.users_size());
        for (const auto &user : response.users()) {
            other_users.append(
                new User(user.id(), QString::fromStdString(user.username())));
        }
    } else {
        lg::write(lg::level::error, "GetOtherUsers({}) -> Err({})",
                  query.toStdString(), status.error_message());
    }
    emit getOtherUsersResponse(status.ok(), other_users);
}

void Backend::requestCreateConversation(int conversation_recv_user_id) {
    grpc::ClientContext context;
    service::main::CreateConversationRequest request;
    request.set_user_id(user_->id());
    request.set_other_user_id(conversation_recv_user_id);
    service::main::CreateConversationResponse response;
    grpc::Status status =
        stub_->CreateConversation(&context, request, &response);
    Conversation *conversation = nullptr;
    if (status.ok()) {
        lg::write(lg::level::info, "CreateConversation({}) -> Ok({},{})",
                  conversation_recv_user_id, response.conversation_id(),
                  response.conversation_recv_user_username());
        set_conversation(new Conversation(
            response.conversation_id(), conversation_recv_user_id,
            QString::fromStdString(response.conversation_recv_user_username()),
            0));
        conversation = new Conversation(
            response.conversation_id(), conversation_recv_user_id,
            QString::fromStdString(response.conversation_recv_user_username()),
            0);
    } else {
        lg::write(lg::level::error, "CreateConversation({}) -> Err({})",
                  conversation_recv_user_id, status.error_message());
    }
    emit createConversationResponse(status.ok(), conversation);
}

void Backend::requestGetConversations() {
    grpc::ClientContext context;
    service::main::GetConversationsRequest request;
    request.set_user_id(user_->id());
    service::main::GetConversationsResponse response;
    grpc::Status status = stub_->GetConversations(&context, request, &response);
    QList<Conversation *> conversations;
    if (status.ok()) {
        lg::write(lg::level::info, "GetConversations({}) -> Ok({})",
                  user_->id(), response.conversations_size());
        for (const auto &conversation : response.conversations()) {
            conversations.append(new Conversation(
                conversation.id(), conversation.recv_user_id(),
                QString::fromStdString(conversation.recv_user_username()), 0));
        }
    } else {
        lg::write(lg::level::error, "GetConversations({}) -> Err({})",
                  user_->id(), status.error_message());
    }
    emit getConversationsResponse(status.ok(), conversations);
}

void Backend::requestDeleteConversation(int conversation_id,
                                        int conversation_index) {
    grpc::ClientContext context;
    service::main::DeleteConversationRequest request;
    request.set_conversation_id(conversation_id);
    service::main::DeleteConversationResponse response;
    grpc::Status status =
        stub_->DeleteConversation(&context, request, &response);
    if (status.ok()) {
        lg::write(lg::level::info, "DeleteConversation({}) -> Ok()",
                  conversation_id);
    } else {
        lg::write(lg::level::error, "DeleteConversation({}) -> Err({})",
                  conversation_id, status.error_message());
    }
    emit deleteConversationResponse(status.ok(), conversation_index);
}

void Backend::requestSendMessage(const QString &message_content) {
    grpc::ClientContext context;
    service::main::SendMessageRequest request;
    request.set_conversation_id(conversation_->id());
    request.set_message_send_user_id(user_->id());
    request.set_message_content(message_content.toStdString());
    service::main::SendMessageResponse response;
    grpc::Status status = stub_->SendMessage(&context, request, &response);
    Message *message = nullptr;
    if (status.ok()) {
        lg::write(lg::level::info, "SendMessage({},{}) -> Ok({})",
                  conversation_->id(), message_content.toStdString(),
                  response.message_id());
    } else {
        lg::write(lg::level::error, "SendMessage({},{}) -> Err({})",
                  conversation_->id(), message_content.toStdString(),
                  status.error_message());
    }
}

void Backend::requestGetMessages() {
    grpc::ClientContext context;
    service::main::GetMessagesRequest request;
    request.set_conversation_id(conversation_->id());
    request.set_last_message_id(0);  // TODO
    service::main::GetMessagesResponse response;
    grpc::Status status = stub_->GetMessages(&context, request, &response);
    QList<Message *> messages;
    if (status.ok()) {
        lg::write(lg::level::info, "GetMessages({}) -> Ok({})",
                  conversation_->id(), response.messages_size());
        for (const auto &message : response.messages()) {
            messages.append(
                new Message(message.id(), message.send_user_id(), true,
                            QString::fromStdString(message.content())));
        }
    } else {
        lg::write(lg::level::error, "GetMessages({}) -> Err({})",
                  conversation_->id(), status.error_message());
    }
    emit getMessagesResponse(status.ok(), conversation_->id(), messages);
}

void Backend::requestDeleteMessage(int message_id, int message_index) {
    grpc::ClientContext context;
    service::main::DeleteMessageRequest request;
    request.set_conversation_id(conversation_->id());
    request.set_message_id(message_id);
    service::main::DeleteMessageResponse response;
    grpc::Status status = stub_->DeleteMessage(&context, request, &response);
    if (status.ok()) {
        lg::write(lg::level::info, "deleteMessage({}) -> Ok()", message_id);
    } else {
        lg::write(lg::level::error, "deleteMessage({}) -> Err({})", message_id,
                  status.error_message());
    }
    emit deleteMessageResponse(status.ok(), message_index);
}
}  // namespace qobject
}  // namespace converse
