#include <gtest/gtest.h>
#include "custom_allocator.h"
#include "custom_container.h"

TEST(AllocatorTest, AllocateNormal) {
    fixed_allocator<int, 10> alloc;
    int* p = alloc.allocate(5);
    ASSERT_NE(p, nullptr);
    *p = 42;
    EXPECT_EQ(*p, 42);
}

TEST(AllocatorTest, AllocateTooMuch) {
    fixed_allocator<int, 10> alloc;
    EXPECT_THROW(alloc.allocate(11), std::bad_alloc);
}

TEST(AllocatorTest, AllocateCumulativeTooMuch) {
    fixed_allocator<int, 10> alloc;
    alloc.allocate(5);
    alloc.allocate(5);
    EXPECT_THROW(alloc.allocate(1), std::bad_alloc);
}

TEST(ContainerTest, AddAndIterate) {
    custom_list<int> c;
    for(int i = 0; i < 5; ++i) c.push_back(i);

    int expected = 0;
    for(auto val : c) {
        EXPECT_EQ(val, expected++);
    }
    EXPECT_EQ(expected, 5);
}

TEST(ContainerWithAllocatorTest, AddAndIterate) {
    custom_list<int, fixed_allocator<int, 10>> c;
    for(int i = 0; i < 10; ++i) c.push_back(i);

    int expected = 0;
    for(auto val : c) {
        EXPECT_EQ(val, expected++);
    }
    EXPECT_EQ(expected, 10);

    EXPECT_THROW(c.push_back(10), std::bad_alloc);
}
