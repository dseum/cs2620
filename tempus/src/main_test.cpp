#define BOOST_TEST_MODULE LogicTests
#include <boost/test/unit_test.hpp>

#include "logger.hpp"
#include "logic.hpp"

class MockLogger : public Logger {
   public:
    MockLogger(int rank) : Logger(rank) {}

    ~MockLogger() {
        // Delete log file
        std::remove("rank_-1.log");
    }
};

MockLogger logger(-1);  // Global logger for testing

BOOST_AUTO_TEST_SUITE(LogicalClockTests)

BOOST_AUTO_TEST_CASE(initial_value_test) {
    LogicalClock clock(5);
    BOOST_CHECK_EQUAL(clock.getValue(), 5);
}

BOOST_AUTO_TEST_CASE(increment_test) {
    LogicalClock clock(10);
    clock.increment();
    BOOST_CHECK_EQUAL(clock.getValue(), 11);
}

BOOST_AUTO_TEST_CASE(update_on_receive_test) {
    LogicalClock clock(5);

    // Test when received timestamp is greater
    clock.updateOnReceive(10);
    BOOST_CHECK_EQUAL(clock.getValue(), 11);

    // Test when current clock is greater
    clock.setValue(20);
    clock.updateOnReceive(15);
    BOOST_CHECK_EQUAL(clock.getValue(), 21);

    // Test when both are equal
    clock.setValue(30);
    clock.updateOnReceive(30);
    BOOST_CHECK_EQUAL(clock.getValue(), 31);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(EventDecisionTests)

BOOST_AUTO_TEST_CASE(decide_next_event_test) {
    // Test sending to next node
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(1, 10)),
                      std::to_underlying(EventType::SEND_TO_NEXT));

    // Test sending to second next node
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(2, 10)),
                      std::to_underlying(EventType::SEND_TO_SECOND_NEXT));

    // Test sending to all nodes
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(3, 10)),
                      std::to_underlying(EventType::SEND_TO_ALL));

    // Test internal event
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(5, 10)),
                      std::to_underlying(EventType::INTERNAL));
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(10, 10)),
                      std::to_underlying(EventType::INTERNAL));

    // Test no event
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(11, 10)),
                      std::to_underlying(EventType::NONE));
}

BOOST_AUTO_TEST_CASE(decide_next_event_with_different_cap) {
    // Test with a different internal event probability cap
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(5, 5)),
                      std::to_underlying(EventType::INTERNAL));
    BOOST_CHECK_EQUAL(std::to_underlying(decideNextEvent(6, 5)),
                      std::to_underlying(EventType::NONE));
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TargetCalculationTests)

BOOST_AUTO_TEST_CASE(calculate_target_rank_test) {
    // Test basic target calculation
    BOOST_CHECK_EQUAL(calculateTargetRank(1, 1, 3), 2);
    BOOST_CHECK_EQUAL(calculateTargetRank(2, 1, 3), 0);

    // Test with larger offset
    BOOST_CHECK_EQUAL(calculateTargetRank(0, 2, 3), 2);
    BOOST_CHECK_EQUAL(calculateTargetRank(1, 2, 3), 0);

    // Test with larger number of VMs
    BOOST_CHECK_EQUAL(calculateTargetRank(3, 2, 6), 5);
    BOOST_CHECK_EQUAL(calculateTargetRank(5, 2, 6), 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ChannelIndexTests)

BOOST_AUTO_TEST_CASE(calculate_channel_index_test) {
    // When target rank is greater than source rank
    BOOST_CHECK_EQUAL(calculateChannelIndex(1, 2), 1);
    BOOST_CHECK_EQUAL(calculateChannelIndex(0, 2), 1);

    // When target rank is less than source rank
    BOOST_CHECK_EQUAL(calculateChannelIndex(2, 0), 0);
    BOOST_CHECK_EQUAL(calculateChannelIndex(3, 1), 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(MessageProcessingTests)

BOOST_AUTO_TEST_CASE(process_received_message_test) {
    LogicalClock clock(5);
    Message msg;
    msg.senderRank = 2;
    msg.logicalTimestamp = 10;

    // Use our test wrapper instead of the real function
    processReceivedMessage(logger, 1, msg, clock, 3);

    // Check that logical clock was updated correctly
    BOOST_CHECK_EQUAL(clock.getValue(), 11);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TimeUtilitiesTests)

BOOST_AUTO_TEST_CASE(system_time_str_format_test) {
    std::string timeStr = getSystemTimeStr();

    // Check that the string is in the format HH:MM:SS
    BOOST_CHECK_EQUAL(timeStr.length(), 8);
    BOOST_CHECK(timeStr[2] == ':');
    BOOST_CHECK(timeStr[5] == ':');
}

BOOST_AUTO_TEST_SUITE_END()

// Integration test for message serialization and deserialization
BOOST_AUTO_TEST_SUITE(MessageSerializationTests)

BOOST_AUTO_TEST_CASE(message_serialization_test) {
    Message originalMsg;
    originalMsg.senderRank = 3;
    originalMsg.logicalTimestamp = 42;

    // Serialize to buffer
    char buffer[sizeof(Message)];
    std::memcpy(buffer, &originalMsg, sizeof(Message));

    // Deserialize back to message
    Message deserializedMsg;
    std::memcpy(&deserializedMsg, buffer, sizeof(Message));

    // Check fields match
    BOOST_CHECK_EQUAL(deserializedMsg.senderRank, originalMsg.senderRank);
    BOOST_CHECK_EQUAL(deserializedMsg.logicalTimestamp,
                      originalMsg.logicalTimestamp);
}

BOOST_AUTO_TEST_SUITE_END()
