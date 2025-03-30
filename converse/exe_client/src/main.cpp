#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <converse/logging/core.hpp>

#include "backend.hpp"

namespace lg = converse::logging;

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Parses CLI options
    QCommandLineParser parser;
    parser.setApplicationDescription("Converse client");
    QCommandLineOption host_opt(QStringList() << "host", "Host", "host",
                                "0.0.0.0");
    parser.addOption(host_opt);
    QCommandLineOption port_opt(QStringList() << "port", "Port", "port",
                                "50051");
    parser.addOption(port_opt);
    parser.process(app);

    QString host = parser.value(host_opt);
    int port = parser.value(port_opt).toInt();

    converse::qobject::Backend backend(host, port);
    converse::qobject::ConversationsModel conversations_model;
    converse::qobject::MessagesModel messages_model;
    converse::qobject::OtherUsersModel other_users_model;

    QObject::connect(&backend, &converse::qobject::Backend::getMessagesResponse,
                     &messages_model,
                     [&](bool ok, int conversation_id,
                         const QList<converse::qobject::Message *> &messages) {
                         if (ok) {
                             messages_model.set(conversation_id, messages);
                         }
                     });
    QObject::connect(
        &backend, &converse::qobject::Backend::getOtherUsersResponse,
        &other_users_model,
        [&](bool ok, const QList<converse::qobject::User *> &other_users) {
            if (ok) {
                other_users_model.set(other_users);
            }
        });
    QObject::connect(
        &backend, &converse::qobject::Backend::addConversation,
        &conversations_model,
        [&](bool ok, converse::qobject::Conversation *conversation) {
            if (ok) {
                conversations_model.prepend(conversation);
            }
        });
    QObject::connect(
        &backend, &converse::qobject::Backend::getConversationsResponse,
        &conversations_model,
        [&](bool ok,
            const QList<converse::qobject::Conversation *> &conversations) {
            if (ok) {
                conversations_model.set(conversations);
            }
        });
    QObject::connect(
        &backend, &converse::qobject::Backend::deleteConversationResponse,
        &conversations_model, [&](bool ok, int conversation_index) {
            if (ok) {
                conversations_model.remove(conversation_index);
            }
        });
    QObject::connect(&backend,
                     &converse::qobject::Backend::deleteMessageResponse,
                     &messages_model, [&](bool ok, int message_index) {
                         if (ok) {
                             messages_model.remove(message_index);
                         }
                     });
    QObject::connect(
        &backend, &converse::qobject::Backend::receiveMessageResponse,
        &messages_model,
        [&](int conversation_id, converse::qobject::Message *message) {
            std::optional<int> curr_conversation_id =
                messages_model.conversation_id();
            if (curr_conversation_id &&
                *curr_conversation_id == conversation_id) {
                messages_model.append(message);
            } else {
                delete message;
            }
        },
        Qt::QueuedConnection);
    QObject::connect(
        &backend, &converse::qobject::Backend::unreadMessage,
        &conversations_model,
        [&](int conversation_id, int message_id) {
            auto [conversation, index] =
                conversations_model.getInstanceById(conversation_id);
            if (conversation) {
                conversation->add_unread_message_id(message_id);
                conversations_model.move(index, 0);
            } else {
                backend.requestGetConversation(conversation_id);
            }
        },
        Qt::QueuedConnection);
    QObject::connect(
        &backend, &converse::qobject::Backend::receiveReadMessagesResponse,
        &conversations_model,
        [&](int conversation_id, const QList<int> &read_message_ids) {
            auto [conversation, index] =
                conversations_model.getInstanceById(conversation_id);
            if (conversation) {
                conversation->remove_unread_message_ids(read_message_ids);
                conversations_model.move(index, 0);
            } else {
                lg::write(lg::level::error,
                          "readMessagesResponse: there should have been a "
                          "conversation with id {}",
                          conversation_id);
            }
        },
        Qt::QueuedConnection);
    QObject::connect(&backend, &converse::qobject::Backend::userChanged,
                     &conversations_model, [&](bool nothing) {
                         if (nothing) {
                             conversations_model.reset();
                         }
                     });
    QObject::connect(&backend, &converse::qobject::Backend::conversationChanged,
                     &messages_model, [&](bool nothing) {
                         if (nothing) {
                             messages_model.reset();
                         }
                     });

    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("conversationsModel",
                                             &conversations_model);
    engine.rootContext()->setContextProperty("messagesModel", &messages_model);
    engine.rootContext()->setContextProperty("otherUsersModel",
                                             &other_users_model);

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Converse/src/main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    lg::init(lg::level::error, lg::sink_type::console);

    int exit_code = app.exec();
    return exit_code;
}
