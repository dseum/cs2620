import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Backend

ApplicationWindow {
    id: page
    width: 800
    height: 600
    minimumWidth: width
    minimumHeight: height
    maximumWidth: width
    maximumHeight: height
    visible: true

    Connections {
        target: backend
        function onUserChanged() {
            if (backend.user === null) {
                stackView.push(signInPage)
            } else {
                backend.mGetConversations()
                stackView.push(homePage)
            }
        }
        function onConversationChanged() {
            console.log("conversation changed")
            backend.mGetMessages()
        }
    }

    StackView {
        id: stackView
        initialItem: signInPage
        anchors.fill: parent
        pushEnter: Transition {}
        pushExit: Transition {}
        popEnter: Transition {}
        popExit: Transition {}
    }

    header: ToolBar {
        visible: stackView.currentItem.objectName === "homePage"
        RowLayout {
            anchors.fill: parent
            Button {
                id: signOutButton
                text: "Sign out"
                onClicked: {
                    signOutButton.enabled = false
                    backend.mSignOut()
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Delete account"
                onClicked: { passwordDialog.open() }
            }
        }
        Connections {
            target: backend
            function onClientSignoutUserResponded(success) {
                signOutButton.enabled = true
            }
        }
    }

    Dialog {
        id: passwordDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        visible: false
        title: "Confirm account deletion"
        modal: true
        anchors.centerIn: parent
        ColumnLayout {
            TextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "Password"
                echoMode: TextInput.Password
                Keys.onReturnPressed: passwordDialog.accepted()
            }
        }
        onAccepted: {
            passwordDialog.enabled = false
            passwordField.enabled = false
            backend.mDeleteUser(passwordField.text)
        }
        onRejected: { passwordField.text = "" }
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Connections {
            target: backend
            function onClientDeleteUserResponded(success) {
                passwordDialog.enabled = true
                passwordField.text = ""
                passwordField.enabled = true
                if (success) {
                    passwordDialog.close()
                }
            }
        }
    }

    Component {
        id: signInPage
        Page {
            objectName: "signInPage"
            ColumnLayout {
                anchors.centerIn: parent
                width: 150
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Sign in"
                    font.bold: true
                }
                Item { Layout.fillWidth: true; height: 5 }
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "Username"
                    Keys.onReturnPressed: signInButton.clicked()
                }
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    Keys.onReturnPressed: signInButton.clicked()
                }
                Item { Layout.fillWidth: true; height: 5 }
                Button {
                    id: signInButton
                    Layout.fillWidth: true
                    text: "Sign in"
                    onClicked: {
                        signInButton.enabled = false
                        usernameField.enabled = false
                        passwordField.enabled = false
                        backend.mSignIn(usernameField.text, passwordField.text)
                    }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Sign up instead"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { stackView.push(signUpPage) }
                    }
                }
                Text {
                    id: errorText
                    visible: false
                    text: "invalid username or password"
                }
            }
            Connections {
                target: backend
                function onClientSigninUserResponded(success) {
                    signInButton.enabled = true
                    usernameField.enabled = true
                    passwordField.enabled = true
                    if (success) {
                        usernameField.text = ""
                        errorText.visible = false
                    } else {
                        errorText.visible = true
                    }
                    passwordField.text = ""
                }
            }
        }
    }

    Component {
        id: signUpPage
        Page {
            objectName: "signUpPage"
            ColumnLayout {
                anchors.centerIn: parent
                width: 150
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Sign up"
                    font.bold: true
                }
                Item { Layout.fillWidth: true; height: 5 }
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "Username"
                    Keys.onReturnPressed: signUpButton.clicked()
                }
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    Keys.onReturnPressed: signUpButton.clicked()
                }
                Item { Layout.fillWidth: true; height: 5 }
                Button {
                    id: signUpButton
                    Layout.fillWidth: true
                    text: "Sign up"
                    onClicked: {
                        signUpButton.enabled = false
                        usernameField.enabled = false
                        passwordField.enabled = false
                        backend.mSignUp(usernameField.text, passwordField.text)
                    }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Sign in instead"
                    MouseArea {
                        id: mouseHyperlinkArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { stackView.push(signInPage) }
                    }
                }
            }
            Connections {
                target: backend
                function onClientSignupUserResponded(success) {
                    signUpButton.enabled = true
                    usernameField.enabled = true
                    passwordField.enabled = true
                    usernameField.text = ""
                    passwordField.text = ""
                }
            }
        }
    }

    Popup {
        id: otherUsersPopup
        modal: true
        focus: true
        anchors.centerIn: parent
        width: parent.width / 2
        height: parent.height / 2
        ColumnLayout {
            width: parent.width
            height: parent.height
            Layout.fillHeight: true
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "Search"
                    Keys.onReturnPressed: searchButton.clicked()
                }
                Button {
                    id: searchButton
                    text: "="
                    onClicked: { backend.mGetOtherUsers(searchField.text) }
                }
            }
            ListView {
                id: otherUsersListView
                clip: true
                Layout.fillHeight: true
                Layout.fillWidth: true
                model: otherUsersModel
                ScrollBar.vertical: ScrollBar { }
                delegate: ItemDelegate {
                    width: otherUsersListView.width
                    height: 60
                    contentItem: Text {
                        text: model.username
                        font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            otherUsersPopup.enabled = false
                            backend.mCreateConversation(model.id)
                        }
                    }
                    Connections {
                        target: backend
                        function onClientCreateConversationResponded(success) {
                            if (success) {
                                otherUsersPopup.close()
                            }
                            otherUsersPopup.enabled = true
                        }
                    }
                }
            }
        }
    }

    Component {
        id: homePage
        Page {
            objectName: "homePage"
            RowLayout {
                anchors.fill: parent
                spacing: 10
                ColumnLayout {
                    Layout.preferredWidth: parent.width / 5
                    Layout.maximumWidth: parent.width / 4
                    Layout.fillHeight: true
                    spacing: 10

                    ListView {
                        id: conversationsListView
                        clip: true
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: conversationsModel
                        ScrollBar.vertical: ScrollBar { }
                        delegate: ItemDelegate {
                            width: conversationsListView.width
                            height: 60
                            contentItem: Text {
                                text: model.recvUserUsername
                                font.bold: true
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onClicked: {
                                    if (mouse.button === Qt.LeftButton) {
                                        backend.setConversation(
                                            model.id,
                                            model.recvUserId,
                                            model.recvUserUsername
                                        )
                                    } else if (mouse.button === Qt.RightButton) {
                                        conversationContextMenu.selectedIndex = index
                                        conversationContextMenu.popup(mouse.screenX, mouse.screenY)
                                    }
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "+"
                        onClicked: {
                            backend.mGetOtherUsers("")
                            otherUsersPopup.open()
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    spacing: 10
                    Rectangle {
                        id: conversationBackground
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "white"
                        Label {
                            anchors.centerIn: parent
                            visible: backend.conversation === null
                            text: "No conversation selected"
                        }
                        ColumnLayout {
                            visible: backend.conversation !== null
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            width: conversationBackground.width
                            height: conversationBackground.height
                            Label {
                                text: {
                                    if (backend.conversation === null)
                                        return "No conversation selected"
                                    return `Conversation ${backend.conversation.id} with ${backend.conversation.recvUserUsername}`
                                }
                                font.bold: true
                            }
                            ListView {
                                id: messagesListView
                                clip: true
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                model: messagesModel
                                ScrollBar.vertical: ScrollBar { }
                                delegate: ItemDelegate {
                                    width: messagesListView.width
                                    contentItem: ColumnLayout {
                                        Label {
                                            text: (model.sendUserId === backend.user.id)
                                                  ? "You"
                                                  : backend.conversation.recvUserUsername
                                            font.bold: true
                                        }
                                        Label { text: model.content }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        acceptedButtons: Qt.RightButton
                                        onClicked: (mouse) => {
                                            if (mouse.button === Qt.RightButton) {
                                                // Store the index of the right-clicked message
                                                messageContextMenu.selectedIndex = index
                                                // Popup the context menu at the mouse's screen position
                                                messageContextMenu.popup(mouse.screenX, mouse.screenY)
                                            }
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                TextField {
                                    id: messageField
                                    Layout.fillWidth: true
                                    placeholderText: "Content"
                                    Keys.onReturnPressed: sendButton.clicked()
                                }
                                Button {
                                    id: sendButton
                                    text: "="
                                    onClicked: {
                                        backend.mSendMessage(messageField.text)
                                        messageField.text = ""
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //Timer {
            //    id: messagePollTimer
            //    interval: 500      // Poll every 1 seconds
            //    repeat: true
            //    running: true
            //    onTriggered: {
            //        if (backend.conversation !== null) {
            //            backend.mSendMessage("")
            //            backend.mGetConversations()
            //        }
            //    }
            //}
            // Context menu for deleting a message
            Menu {
                id: messageContextMenu
                property int selectedIndex: -1  // holds the index of the clicked message
                MenuItem {
                    text: "Delete Message"
                    onTriggered: {
                        if (backend.conversation !== null && messageContextMenu.selectedIndex !== -1) {
                            var messageData = messagesModel.get(messageContextMenu.selectedIndex)
                            backend.mDeleteMessage(messageData.id)
                        }
                    }
                }
            }

            // Listen for the delete response signal from the backend
            Connections {
                target: backend
                function onClientDeleteMessageResponded(success) {
                    if (success && messageContextMenu.selectedIndex !== -1) {
                        // Remove the deleted message from the model.
                        messagesModel.removeMessageAt(messageContextMenu.selectedIndex)
                    } else if (!success) {
                        console.log("Failed to delete message")
                    }
                    // Reset the selection after handling the response.
                    messageContextMenu.selectedIndex = -1
                }
            }

            Menu {
                id: conversationContextMenu
                property int selectedIndex: -1  // holds the index of the clicked conversation
                MenuItem {
                    text: "Delete Conversation"
                    onTriggered: {
                        if (conversationContextMenu.selectedIndex !== -1) {
                            var conversationData = conversationsModel.get(conversationContextMenu.selectedIndex)
                            backend.mDeleteConversation(conversationData.id)
                        }
                    }
                }
            }
            
            Connections {
                target: backend
                function onClientCreateConversationResponded(success, conversation) {
                    if (success) {
                        conversationsModel.append(conversation)
                    } else {
                        console.log("Failed to create conversation")
                    }
                }
            }
    
            Connections {
                target: backend
                function onClientDeleteConversationResponded(success) {
                    if (success && conversationContextMenu.selectedIndex !== -1) {
                        // Remove the deleted conversation from the model.
                        conversationsModel.removeConversationAt(conversationContextMenu.selectedIndex)
                    } else if (!success) {
                        console.log("Failed to delete conversation")
                    }
                    // Reset the selection after handling the response.
                    conversationContextMenu.selectedIndex = -1
                }
            
            }
        }
    }
}

