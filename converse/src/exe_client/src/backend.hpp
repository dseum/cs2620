#pragma once

#include <converse/service/main/main.grpc.pb.h>
#include <qtypes.h>

#include <QList>
#include <QObject>
#include <QtQml>
#include <memory>

#include "converse/service/main/main.pb.h"
#include "reader.hpp"

namespace converse {
namespace qobject {

class User : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QString username READ username CONSTANT)

   public:
    explicit User(int id, const QString &username, QObject *parent = nullptr);

    int id() const;
    QString username() const;

   private:
    int id_;
    QString username_;
};

class Conversation : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(int recvUserId READ recv_user_id CONSTANT)
    Q_PROPERTY(QString recvUserUsername READ recv_user_username CONSTANT)
    Q_PROPERTY(int unreadCount READ unread_count NOTIFY unreadMessageIdsChanged)
    Q_PROPERTY(QSet<int> unreadMessageIds READ unread_message_ids NOTIFY
                   unreadMessageIdsChanged)

   public:
    explicit Conversation(int id, int recv_user_id,
                          const QString &recv_user_username,
                          const QSet<int> &unread_message_ids,
                          QObject *parent = nullptr);

    int id() const;
    int recv_user_id() const;
    QString recv_user_username() const;
    int unread_count() const;
    QSet<int> unread_message_ids() const;

    void set_unread_message_ids(const QSet<int> &value);
    void add_unread_message_id(int message_id);
    void remove_unread_message_ids(const QList<int> &read_message_ids);
    void reset_unread_message_ids();

   signals:
    void unreadMessageIdsChanged();

   private:
    int id_;
    int recv_user_id_;
    QString recv_user_username_;
    QSet<int> unread_message_ids_;
};

class Message : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(int sendUserId READ send_user_id CONSTANT)
    Q_PROPERTY(bool isRead READ is_read CONSTANT)
    Q_PROPERTY(QString content READ content CONSTANT)

   public:
    explicit Message(int id, int send_user_id, bool is_read,
                     const QString &content, QObject *parent = nullptr);

    int id() const;
    int send_user_id() const;
    bool is_read() const;
    QString content() const;

   private:
    int id_;
    int send_user_id_;
    bool is_read_;
    QString content_;
};

class ConversationsModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    QML_SINGLETON

   public:
    enum ConversationRole {
        IdRole = Qt::UserRole,
        RecvUserIdRole = Qt::UserRole + 1,
        RecvUserUsernameRole = Qt::UserRole + 2,
        UnreadCountRole = Qt::UserRole + 3,
        UnreadMessageIdsRole = Qt::UserRole + 4
    };
    Q_ENUM(ConversationRole)

    explicit ConversationsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

   public slots:
    QVariantMap get(int index);
    std::pair<Conversation *, int> getInstanceById(int conversation_id);
    void remove(int index);
    void removeById(int conversation_id);
    void prepend(Conversation *conversation);
    void append(Conversation *conversation);
    void set(const QList<Conversation *> &conversations);
    void move(int from, int to);
    void reset();

   private:
    QList<Conversation *> conversations_;
};

class MessagesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    QML_SINGLETON

   public:
    enum MessageRoles {
        IdRole = Qt::UserRole,
        SendUserIdRole = Qt::UserRole + 1,
        IsReadRole = Qt::UserRole + 2,
        ContentRole = Qt::UserRole + 3
    };
    Q_ENUM(MessageRoles)

    explicit MessagesModel(QObject *parent = nullptr);
    ~MessagesModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    std::optional<int> conversation_id() const;

   public slots:
    QVariantMap get(int index);
    Message *getInstanceById(int message_id);
    void remove(int index);
    void removeById(int message_id);
    void append(Message *message);
    void set(int conversation_id, const QList<Message *> &messages);
    void reset();

   signals:
    void messagesChangedForScroll();

   private:
    std::optional<int> conversation_id_;
    QList<Message *> messages_;
};

class OtherUsersModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    QML_SINGLETON

   public:
    enum OtherUserRoles {
        IdRole = Qt::UserRole,
        UsernameRole = Qt::UserRole + 1
    };
    Q_ENUM(OtherUserRoles)

    explicit OtherUsersModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

   public slots:
    QVariantMap get(int index);
    void append(User *);
    void set(const QList<User *> &other_users);

   private:
    QList<User *> other_users_;
};

class Backend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    QML_SINGLETON

    Q_PROPERTY(User *user READ user WRITE set_user NOTIFY userChanged)
    Q_PROPERTY(Conversation *conversation READ conversation WRITE
                   set_conversation NOTIFY conversationChanged)

   public:
    explicit Backend(const QString &host, int port, QObject *parent = nullptr);

    User *user() const;

    Conversation *conversation() const;

    void set_user(User *value);
    void set_conversation(Conversation *value);

   public slots:
    void setUser(int id, const QString &username);
    void setConversation(int id, int recv_user_id,
                         const QString &recv_user_username,
                         const QList<int> &unread_message_ids);

    void requestSigninUser(const QString &user_username,
                           const QString &user_password);
    void requestSignupUser(const QString &user_username,
                           const QString &user_password);
    void requestSignoutUser();
    void requestDeleteUser(const QString &user_password);
    void requestGetOtherUsers(const QString &query);
    void requestCreateConversation(int conversation_recv_user_id);
    void requestGetConversation(int conversation_id);
    void requestGetConversations();
    void requestDeleteConversation(int conversation_id, int conversation_index);
    void requestSendMessage(const QString &message_content);
    void requestReadMessages(int conversation_id,
                             const QSet<int> &read_message_ids);
    void requestGetMessages();
    void requestDeleteMessage(int message_id, int message_index);

   signals:
    void userChanged(bool nothing);
    void conversationChanged(bool nothing);
    void signinUserResponse(bool ok);
    void signupUserResponse(bool ok);
    void signoutUserResponse(bool ok);
    void deleteUserResponse(bool ok);
    void getOtherUsersResponse(bool ok, const QList<User *> &other_users);
    void createConversationResponse(bool ok);
    void addConversation(bool ok, Conversation *conversation);
    void getConversationsResponse(bool ok,
                                  const QList<Conversation *> &conversations);
    void deleteConversationResponse(bool ok, int conversation_index);
    void getMessagesResponse(bool ok, int conversation_id,
                             const QList<Message *> &messages);
    void deleteMessageResponse(bool ok, int message_index);
    void receiveMessageResponse(int conversation_id, Message *message);
    void receiveReadMessagesResponse(int conversation_id,
                                     const QList<int> &read_message_ids);
    void unreadMessage(int conversation_id, int message_id);

   private:
    std::shared_ptr<grpc::ChannelInterface> channel_;
    std::unique_ptr<service::main::MainService::Stub> stub_;

    grpc::ClientContext ReceiveMessage_context_;
    service::main::ReceiveMessageRequest ReceiveMessage_request_;
    std::unique_ptr<service::main::reader::ReceiveMessageReader>
        ReceiveMessage_reader_;
    grpc::ClientContext ReceiveReadMessages_context_;
    service::main::ReceiveReadMessagesRequest ReceiveReadMessages_request_;
    std::unique_ptr<service::main::reader::ReceiveReadMessagesReader>
        ReceiveReadMessages_reader_;

    User *user_;
    Conversation *conversation_;
};
}  // namespace qobject
}  // namespace converse
