#define BOOST_TEST_MODULE ServerTests

#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <boost/test/unit_test.hpp>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>

#include "gen/rpc/converse/converse.grpc.pb.h"
#include "server.hpp"

using namespace converse;

// TestDb uses the default constructor, which opens "main.db".
class TestDb : public Db {
   public:
    TestDb() : Db() { resetDatabase(); }

    // Reset the schema: drop tables if they exist and recreate them.
    void resetDatabase() {
        // Drop tables if they exist.
        execute("DROP TABLE IF EXISTS users");
        execute("DROP TABLE IF EXISTS conversations");
        execute("DROP TABLE IF EXISTS conversations_users");
        execute("DROP TABLE IF EXISTS messages");

        // Create necessary tables.
        execute(
            "CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "username TEXT, password_encoded TEXT, is_deleted INTEGER DEFAULT "
            "0)");
        execute(
            "CREATE TABLE conversations (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "is_deleted INTEGER DEFAULT 0)");
        execute(
            "CREATE TABLE conversations_users (conversation_id INTEGER, "
            "user_id INTEGER)");
        execute(
            "CREATE TABLE messages (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "conversation_id INTEGER, user_id INTEGER, content TEXT, is_read "
            "INTEGER, is_deleted INTEGER DEFAULT 0, created_at TIMESTAMP "
            "DEFAULT CURRENT_TIMESTAMP)");
    }
};

struct GrpcFixture {
    GrpcFixture() {
        // Create a test database (using main.db via the default Db constructor)
        auto db = std::make_unique<TestDb>();

        // Create the service implementation and assign the database.
        service = std::make_unique<MyConverseServiceImpl>();
        service->setDb(std::move(db));

        // Build and start the gRPC server, specifying the listening port.
        grpc::ServerBuilder builder;
        // This is the key change: add a listening port.
        builder.AddListeningPort("localhost:50051",
                                 grpc::InsecureServerCredentials());
        builder.RegisterService(service.get());
        server = builder.BuildAndStart();

        // Create a client channel.
        channel = grpc::CreateChannel("localhost:50051",
                                      grpc::InsecureChannelCredentials());
        stub = converse::ConverseService::NewStub(channel);
    }

    ~GrpcFixture() { server->Shutdown(); }

    // Convenience method to reset the database before each test.
    void resetDatabase() {
        auto *testDb = dynamic_cast<TestDb *>(service->db_.get());
        if (testDb) {
            testDb->resetDatabase();
        }
    }

    std::unique_ptr<MyConverseServiceImpl> service;
    std::unique_ptr<grpc::Server> server;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<converse::ConverseService::Stub> stub;
};

BOOST_FIXTURE_TEST_SUITE(ServerTests, GrpcFixture)

BOOST_AUTO_TEST_CASE(test_signup_user) {
    resetDatabase();  // Ensure a clean database.
    std::cout << "Database reset for test_signup_user" << std::endl;

    grpc::ClientContext context;
    SignupUserRequest request;
    SignupUserResponse response;

    request.set_username("testuser");
    request.set_password("testpassword");

    grpc::Status status = stub->SignupUser(&context, request, &response);

    BOOST_CHECK(status.ok());
    BOOST_CHECK(response.user_id() > 0);
}

BOOST_AUTO_TEST_CASE(test_signin_user) {
    resetDatabase();
    std::cout << "Database reset for test_signin_user" << std::endl;

    // Sign up a user.
    grpc::ClientContext context1;
    SignupUserRequest signup_request;
    SignupUserResponse signup_response;

    signup_request.set_username("testuser");
    signup_request.set_password("testpassword");

    grpc::Status signup_status =
        stub->SignupUser(&context1, signup_request, &signup_response);
    BOOST_REQUIRE(signup_status.ok());

    // Now sign in the user.
    grpc::ClientContext context2;
    SigninUserRequest signin_request;
    SigninUserResponse signin_response;

    signin_request.set_username("testuser");
    signin_request.set_password("testpassword");

    grpc::Status signin_status =
        stub->SigninUser(&context2, signin_request, &signin_response);

    BOOST_CHECK(signin_status.ok());
    BOOST_CHECK_EQUAL(signin_response.user_id(), signup_response.user_id());
}

BOOST_AUTO_TEST_CASE(test_send_message) {
    resetDatabase();
    std::cout << "Database reset for test_send_message" << std::endl;

    // Sign up two users.
    grpc::ClientContext context1;
    SignupUserRequest signup_request1;
    SignupUserResponse signup_response1;

    signup_request1.set_username("user1");
    signup_request1.set_password("password1");

    grpc::Status signup_status1 =
        stub->SignupUser(&context1, signup_request1, &signup_response1);
    BOOST_REQUIRE(signup_status1.ok());

    grpc::ClientContext context2;
    SignupUserRequest signup_request2;
    SignupUserResponse signup_response2;

    signup_request2.set_username("user2");
    signup_request2.set_password("password2");

    grpc::Status signup_status2 =
        stub->SignupUser(&context2, signup_request2, &signup_response2);
    BOOST_REQUIRE(signup_status2.ok());

    // Create a conversation between the two users.
    grpc::ClientContext context3;
    CreateConversationRequest create_conv_request;
    CreateConversationResponse create_conv_response;

    create_conv_request.set_user_id(signup_response1.user_id());
    create_conv_request.set_other_user_id(signup_response2.user_id());

    grpc::Status create_conv_status = stub->CreateConversation(
        &context3, create_conv_request, &create_conv_response);
    BOOST_REQUIRE(create_conv_status.ok());

    // Send a message from user1 to user2.
    grpc::ClientContext context4;
    SendMessageRequest send_msg_request;
    SendMessageResponse send_msg_response;

    send_msg_request.set_conversation_id(
        create_conv_response.conversation_id());
    send_msg_request.set_send_user_id(signup_response1.user_id());
    send_msg_request.set_content("Hello, user2!");

    grpc::Status send_msg_status =
        stub->SendMessage(&context4, send_msg_request, &send_msg_response);
    BOOST_CHECK(send_msg_status.ok());
}

BOOST_AUTO_TEST_CASE(test_signout_user) {
    // Signout is currently a stub that returns OK.
    grpc::ClientContext context;
    SignoutUserRequest request;
    SignoutUserResponse response;
    grpc::Status status = stub->SignoutUser(&context, request, &response);
    BOOST_CHECK(status.ok());
}

BOOST_AUTO_TEST_CASE(test_delete_user) {
    resetDatabase();

    // Sign up a user.
    grpc::ClientContext ctx_signup;
    SignupUserRequest signup_req;
    SignupUserResponse signup_res;
    signup_req.set_username("deleteuser");
    signup_req.set_password("delpass");
    grpc::Status signup_status =
        stub->SignupUser(&ctx_signup, signup_req, &signup_res);
    BOOST_REQUIRE(signup_status.ok());
    auto user_id = signup_res.user_id();

    // Attempt to delete the user with an incorrect password.
    grpc::ClientContext ctx_wrong;
    DeleteUserRequest del_req_wrong;
    DeleteUserResponse del_res_wrong;
    del_req_wrong.set_user_id(user_id);
    del_req_wrong.set_password("wrongpass");
    grpc::Status del_status_wrong =
        stub->DeleteUser(&ctx_wrong, del_req_wrong, &del_res_wrong);
    BOOST_CHECK(!del_status_wrong.ok());

    // Delete the user with the correct password.
    grpc::ClientContext ctx_del;
    DeleteUserRequest del_req;
    DeleteUserResponse del_res;
    del_req.set_user_id(user_id);
    del_req.set_password("delpass");
    grpc::Status del_status = stub->DeleteUser(&ctx_del, del_req, &del_res);
    BOOST_CHECK(del_status.ok());

    // Attempt to sign in the deleted user; this should fail.
    grpc::ClientContext ctx_signin;
    SigninUserRequest signin_req;
    SigninUserResponse signin_res;
    signin_req.set_username("deleteuser");
    signin_req.set_password("delpass");
    grpc::Status signin_status =
        stub->SigninUser(&ctx_signin, signin_req, &signin_res);
    BOOST_CHECK(!signin_status.ok());
}

BOOST_AUTO_TEST_CASE(test_get_other_users) {
    resetDatabase();

    // Create three users: userA, userB, userC.
    auto createUser = [&](const std::string &username,
                          const std::string &password) -> int64_t {
        grpc::ClientContext ctx;
        SignupUserRequest req;
        SignupUserResponse res;
        req.set_username(username);
        req.set_password(password);
        grpc::Status status = stub->SignupUser(&ctx, req, &res);
        BOOST_REQUIRE(status.ok());
        return res.user_id();
    };

    int64_t idA = createUser("userA", "passA");
    int64_t idB = createUser("userB", "passB");
    int64_t idC = createUser("userC", "passC");

    // Get other users for userA with an empty query.
    grpc::ClientContext ctx;
    GetOtherUsersRequest getReq;
    GetOtherUsersResponse getRes;
    getReq.set_user_id(idA);
    getReq.set_query("");
    getReq.set_limit(10);
    getReq.set_offset(0);
    grpc::Status status = stub->GetOtherUsers(&ctx, getReq, &getRes);
    BOOST_CHECK(status.ok());
    BOOST_CHECK_EQUAL(getRes.users_size(), 2);
    std::vector<int64_t> returned_ids;
    for (const auto &user : getRes.users()) {
        returned_ids.push_back(user.user_id());
    }
    BOOST_CHECK(std::find(returned_ids.begin(), returned_ids.end(), idB) !=
                returned_ids.end());
    BOOST_CHECK(std::find(returned_ids.begin(), returned_ids.end(), idC) !=
                returned_ids.end());

    // Now, query with a filter ("B") to expect only userB.
    grpc::ClientContext ctx2;
    GetOtherUsersRequest getReq2;
    GetOtherUsersResponse getRes2;
    getReq2.set_user_id(idA);
    getReq2.set_query("B");
    getReq2.set_limit(10);
    getReq2.set_offset(0);
    status = stub->GetOtherUsers(&ctx2, getReq2, &getRes2);
    BOOST_CHECK(status.ok());
    BOOST_CHECK_EQUAL(getRes2.users_size(), 1);
    BOOST_CHECK_EQUAL(getRes2.users(0).user_id(), idB);
}

BOOST_AUTO_TEST_CASE(test_create_conversation) {
    resetDatabase();

    // Sign up two users.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_username("conv_user1");
    signup_req1.set_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_username("conv_user2");
    signup_req2.set_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    // Create a conversation.
    grpc::ClientContext ctx3;
    CreateConversationRequest conv_req;
    CreateConversationResponse conv_res;
    conv_req.set_user_id(signup_res1.user_id());
    conv_req.set_other_user_id(signup_res2.user_id());
    grpc::Status status = stub->CreateConversation(&ctx3, conv_req, &conv_res);
    BOOST_REQUIRE(status.ok());
    BOOST_CHECK(conv_res.conversation_id() > 0);
    BOOST_CHECK_EQUAL(conv_res.recv_user_id(), signup_res2.user_id());
    BOOST_CHECK(!conv_res.recv_username().empty());

    // Test error case: attempt to create a conversation with a non-existent
    // other user.
    grpc::ClientContext ctx4;
    CreateConversationRequest conv_req_err;
    CreateConversationResponse conv_res_err;
    conv_req_err.set_user_id(signup_res1.user_id());
    conv_req_err.set_other_user_id(9999);  // non-existent user ID.
    grpc::Status status_err =
        stub->CreateConversation(&ctx4, conv_req_err, &conv_res_err);
    BOOST_CHECK(!status_err.ok());
}

BOOST_AUTO_TEST_CASE(test_get_conversations) {
    resetDatabase();

    // Sign up two users.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_username("conv_get_user1");
    signup_req1.set_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_username("conv_get_user2");
    signup_req2.set_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    // Create a conversation.
    grpc::ClientContext ctx3;
    CreateConversationRequest create_conv_req;
    CreateConversationResponse create_conv_res;
    create_conv_req.set_user_id(signup_res1.user_id());
    create_conv_req.set_other_user_id(signup_res2.user_id());
    BOOST_REQUIRE(
        stub->CreateConversation(&ctx3, create_conv_req, &create_conv_res)
            .ok());

    // Send a message from user1 to user2.
    grpc::ClientContext ctx4;
    SendMessageRequest send_msg_req;
    SendMessageResponse send_msg_res;
    send_msg_req.set_conversation_id(create_conv_res.conversation_id());
    send_msg_req.set_send_user_id(signup_res1.user_id());
    send_msg_req.set_content("Hello from user1");
    BOOST_REQUIRE(stub->SendMessage(&ctx4, send_msg_req, &send_msg_res).ok());

    // Get conversations for user2 (should show an unread count of 1).
    grpc::ClientContext ctx5;
    GetConversationsRequest get_conv_req;
    GetConversationsResponse get_conv_res;
    get_conv_req.set_user_id(signup_res2.user_id());
    BOOST_REQUIRE(
        stub->GetConversations(&ctx5, get_conv_req, &get_conv_res).ok());
    BOOST_CHECK_EQUAL(get_conv_res.conversations_size(), 1);
    auto conv = get_conv_res.conversations(0);
    BOOST_CHECK_EQUAL(conv.conversation_id(),
                      create_conv_res.conversation_id());

    // Get conversations for user1 (should show an unread count of 0).
    grpc::ClientContext ctx6;
    GetConversationsRequest get_conv_req2;
    GetConversationsResponse get_conv_res2;
    get_conv_req2.set_user_id(signup_res1.user_id());
    BOOST_REQUIRE(
        stub->GetConversations(&ctx6, get_conv_req2, &get_conv_res2).ok());
    BOOST_CHECK_EQUAL(get_conv_res2.conversations_size(), 1);
    auto conv2 = get_conv_res2.conversations(0);
    BOOST_CHECK_EQUAL(conv2.unread_count(), 0);
}

BOOST_AUTO_TEST_CASE(test_delete_conversation) {
    resetDatabase();

    // Sign up two users and create a conversation.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_username("delconv_user1");
    signup_req1.set_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_username("delconv_user2");
    signup_req2.set_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    grpc::ClientContext ctx3;
    CreateConversationRequest conv_req;
    CreateConversationResponse conv_res;
    conv_req.set_user_id(signup_res1.user_id());
    conv_req.set_other_user_id(signup_res2.user_id());
    BOOST_REQUIRE(stub->CreateConversation(&ctx3, conv_req, &conv_res).ok());

    // Delete the conversation.
    grpc::ClientContext ctx4;
    DeleteConversationRequest del_conv_req;
    DeleteConversationResponse del_conv_res;
    del_conv_req.set_conversation_id(conv_res.conversation_id());
    BOOST_REQUIRE(
        stub->DeleteConversation(&ctx4, del_conv_req, &del_conv_res).ok());

    // Get conversations for user1; the deleted conversation should not be
    // returned.
    grpc::ClientContext ctx5;
    GetConversationsRequest get_conv_req;
    GetConversationsResponse get_conv_res;
    get_conv_req.set_user_id(signup_res1.user_id());
    BOOST_REQUIRE(
        stub->GetConversations(&ctx5, get_conv_req, &get_conv_res).ok());
    BOOST_CHECK_EQUAL(get_conv_res.conversations_size(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_messages) {
    resetDatabase();

    // Sign up two users and create a conversation.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_username("msg_user1");
    signup_req1.set_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_username("msg_user2");
    signup_req2.set_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    grpc::ClientContext ctx3;
    CreateConversationRequest conv_req;
    CreateConversationResponse conv_res;
    conv_req.set_user_id(signup_res1.user_id());
    conv_req.set_other_user_id(signup_res2.user_id());
    BOOST_REQUIRE(stub->CreateConversation(&ctx3, conv_req, &conv_res).ok());

    // Send two messages.
    grpc::ClientContext ctx4;
    SendMessageRequest send_msg_req1;
    SendMessageResponse send_msg_res1;
    send_msg_req1.set_conversation_id(conv_res.conversation_id());
    send_msg_req1.set_send_user_id(signup_res1.user_id());
    send_msg_req1.set_content("First message");
    BOOST_REQUIRE(stub->SendMessage(&ctx4, send_msg_req1, &send_msg_res1).ok());

    grpc::ClientContext ctx5;
    SendMessageRequest send_msg_req2;
    SendMessageResponse send_msg_res2;
    send_msg_req2.set_conversation_id(conv_res.conversation_id());
    send_msg_req2.set_send_user_id(signup_res2.user_id());
    send_msg_req2.set_content("Second message");
    BOOST_REQUIRE(stub->SendMessage(&ctx5, send_msg_req2, &send_msg_res2).ok());

    // Retrieve messages.
    grpc::ClientContext ctx6;
    GetMessagesRequest get_msgs_req;
    GetMessagesResponse get_msgs_res;
    get_msgs_req.set_conversation_id(conv_res.conversation_id());
    BOOST_REQUIRE(stub->GetMessages(&ctx6, get_msgs_req, &get_msgs_res).ok());
    BOOST_CHECK(get_msgs_res.messages_size() >= 2);
    BOOST_CHECK_EQUAL(get_msgs_res.messages(0).content(), "First message");
    BOOST_CHECK_EQUAL(get_msgs_res.messages(1).content(), "Second message");
}

BOOST_AUTO_TEST_CASE(test_delete_message) {
    resetDatabase();

    // Sign up two users and create a conversation.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_username("delmsg_user1");
    signup_req1.set_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_username("delmsg_user2");
    signup_req2.set_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    grpc::ClientContext ctx3;
    CreateConversationRequest conv_req;
    CreateConversationResponse conv_res;
    conv_req.set_user_id(signup_res1.user_id());
    conv_req.set_other_user_id(signup_res2.user_id());
    BOOST_REQUIRE(stub->CreateConversation(&ctx3, conv_req, &conv_res).ok());

    // Send a message.
    grpc::ClientContext ctx4;
    SendMessageRequest send_msg_req;
    SendMessageResponse send_msg_res;
    send_msg_req.set_conversation_id(conv_res.conversation_id());
    send_msg_req.set_send_user_id(signup_res1.user_id());
    send_msg_req.set_content("Message to be deleted");
    BOOST_REQUIRE(stub->SendMessage(&ctx4, send_msg_req, &send_msg_res).ok());

    // Get messages to obtain the message ID.
    grpc::ClientContext ctx5;
    GetMessagesRequest get_msgs_req;
    GetMessagesResponse get_msgs_res;
    get_msgs_req.set_conversation_id(conv_res.conversation_id());
    BOOST_REQUIRE(stub->GetMessages(&ctx5, get_msgs_req, &get_msgs_res).ok());
    BOOST_CHECK(get_msgs_res.messages_size() > 0);
    int64_t message_id = get_msgs_res.messages(0).message_id();

    // Delete the message.
    grpc::ClientContext ctx6;
    DeleteMessageRequest del_msg_req;
    DeleteMessageResponse del_msg_res;
    del_msg_req.set_conversation_id(conv_res.conversation_id());
    del_msg_req.set_message_id(message_id);
    BOOST_REQUIRE(stub->DeleteMessage(&ctx6, del_msg_req, &del_msg_res).ok());

    // Get messages again and ensure the deleted message does not appear.
    grpc::ClientContext ctx7;
    GetMessagesRequest get_msgs_req2;
    GetMessagesResponse get_msgs_res2;
    get_msgs_req2.set_conversation_id(conv_res.conversation_id());
    BOOST_REQUIRE(stub->GetMessages(&ctx7, get_msgs_req2, &get_msgs_res2).ok());
    for (int i = 0; i < get_msgs_res2.messages_size(); i++) {
        BOOST_CHECK(get_msgs_res2.messages(i).message_id() != message_id);
    }
}

BOOST_AUTO_TEST_CASE(test_receive_message) {
    resetDatabase();

    // Sign up two users and create a conversation.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_username("stream_user1");
    signup_req1.set_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_username("stream_user2");
    signup_req2.set_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    grpc::ClientContext ctx3;
    CreateConversationRequest conv_req;
    CreateConversationResponse conv_res;
    conv_req.set_user_id(signup_res1.user_id());
    conv_req.set_other_user_id(signup_res2.user_id());
    BOOST_REQUIRE(stub->CreateConversation(&ctx3, conv_req, &conv_res).ok());

    // Start a streaming call for user2 (receiver).
    converse::ReceiveMessageRequest stream_req;
    stream_req.set_user_id(signup_res2.user_id());
    grpc::ClientContext stream_ctx;
    std::unique_ptr<grpc::ClientReader<converse::MessageWithConversationId>>
        reader(stub->ReceiveMessage(&stream_ctx, stream_req));

    std::vector<converse::MessageWithConversationId> received_msgs;
    std::mutex mtx;
    std::condition_variable cv;
    bool got_message = false;

    // Start a thread to read messages.
    std::thread reader_thread([&]() {
        converse::MessageWithConversationId msg;
        while (reader->Read(&msg)) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                received_msgs.push_back(msg);
                got_message = true;
            }
            cv.notify_one();
        }
    });

    // Give some time for the stream to be established.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send a message from user1 to user2.
    grpc::ClientContext ctx4;
    SendMessageRequest send_msg_req;
    SendMessageResponse send_msg_res;
    send_msg_req.set_conversation_id(conv_res.conversation_id());
    send_msg_req.set_send_user_id(signup_res1.user_id());
    send_msg_req.set_content("Streaming message");
    BOOST_REQUIRE(stub->SendMessage(&ctx4, send_msg_req, &send_msg_res).ok());

    // Wait for the message to be received (or timeout).
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::seconds(2),
                    [&]() { return got_message; });
    }

    // Cancel the stream to end the reading loop.
    stream_ctx.TryCancel();
    reader_thread.join();

    BOOST_CHECK(!received_msgs.empty());
    bool found = false;
    for (const auto &m : received_msgs) {
        if (m.message().content() == "Streaming message") {
            found = true;
            break;
        }
    }
    BOOST_CHECK(found);
}

BOOST_AUTO_TEST_SUITE_END()