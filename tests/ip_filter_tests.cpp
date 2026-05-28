//
// Created by miralius on 27.05.2026.
//

#include "ip_filter_utils.h"
#include <gtest/gtest.h>

namespace {

IPpool make_pool() {
    return {
        {"1", "2", "3", "4"},
        {"1", "99", "3", "4"},
        {"46", "70", "1", "1"},
        {"46", "70", "46", "0"},
        {"5", "6", "7", "8"},
        {"255", "0", "0", "255"},
        {"4", "0", "0", "255"}
    };
}

} // namespace

TEST(FilterByFirstByte, ReturnsOnlyMatchingAddresses) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::false_type{}, 1);

    const IPpool expected = {
        {"1", "2", "3", "4"},
        {"1", "99", "3", "4"}
    };
    EXPECT_EQ(filtered, expected);
}

TEST(FilterByFirstByte, ReturnsEmptyWhenNoMatches) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::false_type{}, 42);

    EXPECT_TRUE(filtered.empty());
}

TEST(FilterByFirstByte, ReturnsEmptyForEmptyInput) {
    const IPpool ip_pool;

    const auto filtered = filter(ip_pool, std::false_type{}, 1);

    EXPECT_TRUE(filtered.empty());
}

TEST(FilterByFirstSecondByte, ReturnsOnlyMatchingAddresses) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::false_type{}, 46, 70);

    const IPpool expected = {
        {"46", "70", "1", "1"},
        {"46", "70", "46", "0"}
    };
    EXPECT_EQ(filtered, expected);
}

TEST(FilterByFirstSecondByte, ReturnsEmptyWhenNoMatches) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::false_type{}, 46, 71);

    EXPECT_TRUE(filtered.empty());
}

TEST(FilterByFirstSecondByte, ReturnsEmptyForEmptyInput) {
    const IPpool ip_pool;

    const auto filtered = filter(ip_pool, std::false_type{}, 46, 70);

    EXPECT_TRUE(filtered.empty());
}

TEST(FilterAny, FindsByteInAnyPosition) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::true_type{}, 4);

    const IPpool expected = {
        {"1", "2", "3", "4"},
        {"1", "99", "3", "4"},
        {"4", "0", "0", "255"}
    };
    EXPECT_EQ(filtered, expected);
}

TEST(FilterAny, FindsBoundaryByte255) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::true_type{}, 255);

    const IPpool expected = {
        {"255", "0", "0", "255"},
        {"4", "0", "0", "255"}
    };
    EXPECT_EQ(filtered, expected);
}

TEST(FilterAny, ReturnsEmptyWhenNoMatches) {
    const auto ip_pool = make_pool();

    const auto filtered = filter(ip_pool, std::true_type{}, 200);

    EXPECT_TRUE(filtered.empty());
}

TEST(FilterAny, ReturnsEmptyForEmptyInput) {
    const IPpool ip_pool;

    const auto filtered = filter(ip_pool, std::true_type{}, 46);

    EXPECT_TRUE(filtered.empty());
}