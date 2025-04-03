#define BOOST_TEST_MODULE ServerTests

#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <memory>

#include "service/main/impl.hpp"

using namespace converse;
using namespace converse::service::main;

struct GrpcFixture {
    GrpcFixture() {
        // Create the service implementation and assign the database.
        std::set<grpc::ServerWriter<service::link::GetTransactionsResponse> *>
            cluster_writers;
        std::shared_mutex cluster_writers_mutex;
        service = std::make_unique<service::main::Impl>(
            "test_service", server::Address("localhost", 50051),
            cluster_writers, cluster_writers_mutex);

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
        stub = std::make_unique<MainService::Stub>(channel);
    }

    ~GrpcFixture() { server->Shutdown(); }

    std::unique_ptr<service::main::Impl> service;
    std::unique_ptr<grpc::Server> server;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<service::main::MainService::Stub> stub;
};

BOOST_FIXTURE_TEST_SUITE(ServerTests, GrpcFixture)

BOOST_AUTO_TEST_CASE(test_signup_user) {
    std::cout << "Database reset for test_signup_user" << std::endl;

    grpc::ClientContext context;
    SignupUserRequest request;
    SignupUserResponse response;

    request.set_user_username("testuser");
    request.set_user_password("testpassword");

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

    signup_request.set_user_username("testuser");
    signup_request.set_user_password("testpassword");

    grpc::Status signup_status =
        stub->SignupUser(&context1, signup_request, &signup_response);
    BOOST_REQUIRE(signup_status.ok());

    // Now sign in the user.
    grpc::ClientContext context2;
    SigninUserRequest signin_request;
    SigninUserResponse signin_response;

    signin_request.set_user_username("testuser");
    signin_request.set_user_password("testpassword");

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

    signup_request1.set_user_username("user1");
    signup_request1.set_user_password("password1");

    grpc::Status signup_status1 =
        stub->SignupUser(&context1, signup_request1, &signup_response1);
    BOOST_REQUIRE(signup_status1.ok());

    grpc::ClientContext context2;
    SignupUserRequest signup_request2;
    SignupUserResponse signup_response2;

    signup_request2.set_user_username("user2");
    signup_request2.set_user_password("password2");

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
        create_conv_response.conversation().id());
    send_msg_request.set_message_send_user_id(signup_response1.user_id());
    send_msg_request.set_message_content("Hello, user2!");

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
    signup_req.set_user_username("deleteuser");
    signup_req.set_user_password("delpass");
    grpc::Status signup_status =
        stub->SignupUser(&ctx_signup, signup_req, &signup_res);
    BOOST_REQUIRE(signup_status.ok());
    auto user_id = signup_res.user_id();

    // Attempt to delete the user with an incorrect password.
    grpc::ClientContext ctx_wrong;
    DeleteUserRequest del_req_wrong;
    DeleteUserResponse del_res_wrong;
    del_req_wrong.set_user_id(user_id);
    del_req_wrong.set_user_password("wrongpass");
    grpc::Status del_status_wrong =
        stub->DeleteUser(&ctx_wrong, del_req_wrong, &del_res_wrong);
    BOOST_CHECK(!del_status_wrong.ok());

    // Delete the user with the correct password.
    grpc::ClientContext ctx_del;
    DeleteUserRequest del_req;
    DeleteUserResponse del_res;
    del_req.set_user_id(user_id);
    del_req.set_user_password("delpass");
    grpc::Status del_status = stub->DeleteUser(&ctx_del, del_req, &del_res);
    BOOST_CHECK(del_status.ok());

    // Attempt to sign in the deleted user; this should fail.
    grpc::ClientContext ctx_signin;
    SigninUserRequest signin_req;
    SigninUserResponse signin_res;
    signin_req.set_user_username("deleteuser");
    signin_req.set_user_password("delpass");
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
        req.set_user_username(username);
        req.set_user_password(password);
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
        returned_ids.push_back(user.id());
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
    BOOST_CHECK_EQUAL(getRes2.users(0).id(), idB);
}

BOOST_AUTO_TEST_CASE(test_create_conversation) {
    resetDatabase();

    // Sign up two users.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_user_username("conv_user1");
    signup_req1.set_user_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_user_username("conv_user2");
    signup_req2.set_user_password("pass2");
    BOOST_REQUIRE(stub->SignupUser(&ctx2, signup_req2, &signup_res2).ok());

    // Create a conversation.
    grpc::ClientContext ctx3;
    CreateConversationRequest conv_req;
    CreateConversationResponse conv_res;
    conv_req.set_user_id(signup_res1.user_id());
    conv_req.set_other_user_id(signup_res2.user_id());
    grpc::Status status = stub->CreateConversation(&ctx3, conv_req, &conv_res);
    BOOST_REQUIRE(status.ok());
    BOOST_CHECK(conv_res.conversation().id() > 0);
    BOOST_CHECK_EQUAL(conv_res.conversation().recv_user_id(),
                      signup_res2.user_id());
    BOOST_CHECK(!conv_res.conversation().recv_user_username().empty());

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
    signup_req1.set_user_username("conv_get_user1");
    signup_req1.set_user_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_user_username("conv_get_user2");
    signup_req2.set_user_password("pass2");
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
    send_msg_req.set_conversation_id(create_conv_res.conversation().id());
    send_msg_req.set_message_send_user_id(signup_res1.user_id());
    send_msg_req.set_message_content("Hello from user1");
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
    BOOST_CHECK_EQUAL(conv.id(), create_conv_res.conversation().id());

    // Get conversations for user1 (should show an unread count of 0).
    grpc::ClientContext ctx6;
    GetConversationsRequest get_conv_req2;
    GetConversationsResponse get_conv_res2;
    get_conv_req2.set_user_id(signup_res1.user_id());
    BOOST_REQUIRE(
        stub->GetConversations(&ctx6, get_conv_req2, &get_conv_res2).ok());
    BOOST_CHECK_EQUAL(get_conv_res2.conversations_size(), 1);
    auto conv2 = get_conv_res2.conversations(0);
    BOOST_CHECK_EQUAL(conv2.unread_message_ids_size(), 0);
}

BOOST_AUTO_TEST_CASE(test_delete_conversation) {
    resetDatabase();

    // Sign up two users and create a conversation.
    grpc::ClientContext ctx1;
    SignupUserRequest signup_req1;
    SignupUserResponse signup_res1;
    signup_req1.set_user_username("delconv_user1");
    signup_req1.set_user_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_user_username("delconv_user2");
    signup_req2.set_user_password("pass2");
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
    del_conv_req.set_conversation_id(conv_res.conversation().id());
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
    signup_req1.set_user_username("msg_user1");
    signup_req1.set_user_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_user_username("msg_user2");
    signup_req2.set_user_password("pass2");
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
    send_msg_req1.set_conversation_id(conv_res.conversation().id());
    send_msg_req1.set_message_send_user_id(signup_res1.user_id());
    send_msg_req1.set_message_content("First message");
    BOOST_REQUIRE(stub->SendMessage(&ctx4, send_msg_req1, &send_msg_res1).ok());

    grpc::ClientContext ctx5;
    SendMessageRequest send_msg_req2;
    SendMessageResponse send_msg_res2;
    send_msg_req2.set_conversation_id(conv_res.conversation().id());
    send_msg_req2.set_message_send_user_id(signup_res2.user_id());
    send_msg_req2.set_message_content("Second message");
    BOOST_REQUIRE(stub->SendMessage(&ctx5, send_msg_req2, &send_msg_res2).ok());

    // Retrieve messages.
    grpc::ClientContext ctx6;
    GetMessagesRequest get_msgs_req;
    GetMessagesResponse get_msgs_res;
    get_msgs_req.set_conversation_id(conv_res.conversation().id());
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
    signup_req1.set_user_username("delmsg_user1");
    signup_req1.set_user_password("pass1");
    BOOST_REQUIRE(stub->SignupUser(&ctx1, signup_req1, &signup_res1).ok());

    grpc::ClientContext ctx2;
    SignupUserRequest signup_req2;
    SignupUserResponse signup_res2;
    signup_req2.set_user_username("delmsg_user2");
    signup_req2.set_user_password("pass2");
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
    send_msg_req.set_conversation_id(conv_res.conversation().id());
    send_msg_req.set_message_send_user_id(signup_res1.user_id());
    send_msg_req.set_message_content("Message to be deleted");
    BOOST_REQUIRE(stub->SendMessage(&ctx4, send_msg_req, &send_msg_res).ok());

    // Get messages to obtain the message ID.
    grpc::ClientContext ctx5;
    GetMessagesRequest get_msgs_req;
    GetMessagesResponse get_msgs_res;
    get_msgs_req.set_conversation_id(conv_res.conversation().id());
    BOOST_REQUIRE(stub->GetMessages(&ctx5, get_msgs_req, &get_msgs_res).ok());
    BOOST_CHECK(get_msgs_res.messages_size() > 0);
    int64_t message_id = get_msgs_res.messages(0).id();

    // Delete the message.
    grpc::ClientContext ctx6;
    DeleteMessageRequest del_msg_req;
    DeleteMessageResponse del_msg_res;
    del_msg_req.set_conversation_id(conv_res.conversation().id());
    del_msg_req.set_message_id(message_id);
    BOOST_REQUIRE(stub->DeleteMessage(&ctx6, del_msg_req, &del_msg_res).ok());

    // Get messages again and ensure the deleted message does not appear.
    grpc::ClientContext ctx7;
    GetMessagesRequest get_msgs_req2;
    GetMessagesResponse get_msgs_res2;
    get_msgs_req2.set_conversation_id(conv_res.conversation().id());
    BOOST_REQUIRE(stub->GetMessages(&ctx7, get_msgs_req2, &get_msgs_res2).ok());
    for (int i = 0; i < get_msgs_res2.messages_size(); i++) {
        BOOST_CHECK(get_msgs_res2.messages(i).id() != message_id);
    }
}

BOOST_AUTO_TEST_SUITE_END()
