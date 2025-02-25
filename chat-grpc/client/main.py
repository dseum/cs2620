import sys
import argparse
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine, qmlRegisterType
from PySide6.QtCore import Qt
from backend import *  # noqa: F403
from backend import Backend, ConversationsModel, MessagesModel, OtherUsersModel


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="QML + Python Application")
    parser.add_argument("--host", default="0.0.0.0", type=str)
    parser.add_argument("--port", default="54100", type=int)

    # Parse the arguments
    args = parser.parse_args()

    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()

    qmlRegisterType(Backend, "Backend", 1, 0, "Backend")
    qmlRegisterType(ConversationsModel, "Backend", 1, 0, "ConversationsModel")
    qmlRegisterType(MessagesModel, "Backend", 1, 0, "MessagesModel")
    qmlRegisterType(OtherUsersModel, "Backend", 1, 0, "OtherUsersModel")

    backend = Backend(args.host, args.port)
    conversations_model = ConversationsModel()
    messages_model = MessagesModel()
    other_users_model = OtherUsersModel()
    backend.clientGetConversationsResponded.connect(
        lambda _, conversations: conversations_model.reset(conversations)
    )
    backend.clientGetMessagesResponded.connect(
        lambda _, messages: messages_model.reset(messages)
    )
    backend.clientGetOtherUsersResponded.connect(
        lambda _, users: other_users_model.reset(users)
    )
    backend.serverSendMessageRequested.connect(
        messages_model.append, Qt.BlockingQueuedConnection
    )
    engine.rootContext().setContextProperty("backend", backend)
    engine.rootContext().setContextProperty("conversationsModel", conversations_model)
    engine.rootContext().setContextProperty("messagesModel", messages_model)
    engine.rootContext().setContextProperty("otherUsersModel", other_users_model)

    engine.load("main.qml")
    if not engine.rootObjects():
        sys.exit(-1)
    exit_code = app.exec()
    del engine
    sys.exit(exit_code)
