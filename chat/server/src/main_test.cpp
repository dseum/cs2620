#define BOOST_TEST_MODULE Tests

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "database.hpp"
#include "protocols/utils.hpp"

BOOST_AUTO_TEST_SUITE(Tests)

// ---------------- Basic tests for utils.hpp
// ---------------------------------------------------

BOOST_AUTO_TEST_CASE(test_serialize_client_operation_code) {
    std::vector<uint8_t> data;
    OperationCodeWrapper::serialize_to(data, OperationCode::CLIENT_SIGNUP_USER);
    BOOST_CHECK(
        std::equal(data.begin(), data.end(), std::vector<uint8_t>{0}.begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_client_operation_code) {
    std::vector<uint8_t> data{0};
    auto [value, offset] = OperationCodeWrapper::deserialize_from(data, 0);
    BOOST_CHECK(value == OperationCode::CLIENT_SIGNUP_USER);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_serialize_server_operation_code) {
    std::vector<uint8_t> data;
    OperationCodeWrapper::serialize_to(data,
                                       OperationCode::SERVER_SEND_MESSAGE);
    BOOST_CHECK(
        std::equal(data.begin(), data.end(), std::vector<uint8_t>{11}.begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_server_operation_code) {
    std::vector<uint8_t> data{11};
    auto [value, offset] = OperationCodeWrapper::deserialize_from(data, 0);
    BOOST_CHECK(value == OperationCode::SERVER_SEND_MESSAGE);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_serialize_status_code) {
    std::vector<uint8_t> data;
    StatusCodeWrapper::serialize_to(data, StatusCode::SUCCESS);
    BOOST_CHECK(
        std::equal(data.begin(), data.end(), std::vector<uint8_t>{0}.begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_status_code) {
    std::vector<uint8_t> data{0};
    auto [value, offset] = StatusCodeWrapper::deserialize_from(data, 0);
    BOOST_CHECK(value == StatusCode::SUCCESS);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_serialize_bool) {
    std::vector<uint8_t> data;
    BoolWrapper::serialize_to(data, true);
    BOOST_CHECK(
        std::equal(data.begin(), data.end(), std::vector<uint8_t>{1}.begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_bool) {
    std::vector<uint8_t> data{1};
    auto [value, offset] = BoolWrapper::deserialize_from(data, 0);
    BOOST_CHECK_EQUAL(value, true);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_serialize_uint8) {
    std::vector<uint8_t> data;
    Uint8Wrapper::serialize_to(data, 1);
    BOOST_CHECK(
        std::equal(data.begin(), data.end(), std::vector<uint8_t>{1}.begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_uint8) {
    std::vector<uint8_t> data{1};
    auto [value, offset] = Uint8Wrapper::deserialize_from(data, 0);
    BOOST_CHECK_EQUAL(value, 1);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_serialize_uint64) {
    std::vector<uint8_t> data;
    Uint64Wrapper::serialize_to(data, 1);
    BOOST_CHECK(
        std::equal(data.begin(), data.end(),
                   std::vector<uint8_t>{0, 0, 0, 0, 0, 0, 0, 1}.begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_uint64) {
    std::vector<uint8_t> data{0, 0, 0, 0, 0, 0, 0, 1};
    auto [value, offset] = Uint64Wrapper::deserialize_from(data, 0);
    BOOST_CHECK_EQUAL(value, 1);
    BOOST_CHECK_EQUAL(offset, 8);
}

BOOST_AUTO_TEST_CASE(test_serialize_string) {
    std::vector<uint8_t> data;
    StringWrapper::serialize_to(data, "hello");
    BOOST_CHECK(std::equal(
        data.begin(), data.end(),
        std::vector<uint8_t>{0, 0, 0, 0, 0, 0, 0, 5, 'h', 'e', 'l', 'l', 'o'}
            .begin()));
}

BOOST_AUTO_TEST_CASE(test_deserialize_string) {
    std::vector<uint8_t> data{0, 0, 0, 0, 0, 0, 0, 5, 'h', 'e', 'l', 'l', 'o'};
    std::vector<uint8_t>::size_type offset = 0;
    auto [value, offset1] = StringWrapper::deserialize_from(data, offset);
    BOOST_CHECK(value == "hello");
    BOOST_CHECK_EQUAL(offset1, data.size());
}

// ---------------- Edge Case Tests
// ---------------------------------------------------

BOOST_AUTO_TEST_CASE(test_string_wrapper_empty) {
    // Test that an empty string is serialized correctly.
    std::vector<uint8_t> data;
    std::string empty = "";
    StringWrapper::serialize_to(data, empty);

    // Expected: 8 bytes for the length (0) and no characters.
    std::vector<uint8_t> expected{0, 0, 0, 0, 0, 0, 0, 0};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected.begin(),
                                  expected.end());

    std::vector<uint8_t>::size_type offset = 0;
    auto [value, new_offset] = StringWrapper::deserialize_from(data, offset);
    BOOST_CHECK_EQUAL(value, empty);
    BOOST_CHECK_EQUAL(new_offset, 8);
}

BOOST_AUTO_TEST_CASE(test_string_wrapper_long_string) {
    // Test serialization of a long string (e.g., 1024 'a's).
    std::string long_str(1024, 'a');
    std::vector<uint8_t> data;
    StringWrapper::serialize_to(data, long_str);

    // Check that the first 8 bytes represent the length (1024).
    std::vector<uint8_t> expected_length = {0, 0, 0, 0,
                                            0, 0, 4, 0};  // 1024 =0x00000400
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.begin() + 8,
                                  expected_length.begin(),
                                  expected_length.end());

    std::vector<uint8_t>::size_type offset = 0;
    auto [value, new_offset] = StringWrapper::deserialize_from(data, offset);
    BOOST_CHECK_EQUAL(value, long_str);
    BOOST_CHECK_EQUAL(new_offset, data.size());
}

BOOST_AUTO_TEST_CASE(test_uint64_wrapper_max_value) {
    // Test serialization/deserialization of the maximum uint64_t value.
    std::vector<uint8_t> data;
    uint64_t max_val = std::numeric_limits<uint64_t>::max();
    Uint64Wrapper::serialize_to(data, max_val);

    std::vector<uint8_t> expected = {0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFF};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected.begin(),
                                  expected.end());

    auto [value, offset] = Uint64Wrapper::deserialize_from(data, 0);
    BOOST_CHECK_EQUAL(value, max_val);
    BOOST_CHECK_EQUAL(offset, 8);
}

BOOST_AUTO_TEST_CASE(test_bool_wrapper_false) {
    // Test serialization/deserialization of the boolean value 'false'.
    std::vector<uint8_t> data;
    BoolWrapper::serialize_to(data, false);
    BOOST_CHECK_EQUAL(data.size(), 1);
    BOOST_CHECK_EQUAL(data[0], 0);

    auto [value, offset] = BoolWrapper::deserialize_from(data, 0);
    BOOST_CHECK_EQUAL(value, false);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_uint8_wrapper_zero) {
    // Test serialization/deserialization of 0 for Uint8Wrapper.
    std::vector<uint8_t> data;
    Uint8Wrapper::serialize_to(data, 0);
    BOOST_CHECK_EQUAL(data.size(), 1);
    BOOST_CHECK_EQUAL(data[0], 0);

    auto [value, offset] = Uint8Wrapper::deserialize_from(data, 0);
    BOOST_CHECK_EQUAL(value, 0);
    BOOST_CHECK_EQUAL(offset, 1);
}

BOOST_AUTO_TEST_CASE(test_string_wrapper_size_not_implemented) {
    // Test that calling the non-implemented StringWrapper::size() throws a
    // logic_error.
    BOOST_CHECK_THROW(StringWrapper::size(), std::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()
