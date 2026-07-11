#include <gtest/gtest.h>

#include "matrix.hpp"

TEST(Matrix, StoresOnlyNonDefaultValues) {
    Matrix matrix(-1);
    EXPECT_EQ(matrix.size(), 0u);
    EXPECT_EQ(matrix[10][20], -1);

    matrix[10][20] = 42;
    EXPECT_EQ(matrix[10][20], 42);
    EXPECT_EQ(matrix.size(), 1u);

    matrix[10][20] = -1;
    EXPECT_EQ(matrix.size(), 0u);
}

TEST(Matrix, IteratesOverOccupiedCells) {
    Matrix matrix;
    ((matrix[100][100] = 314) = -1) = 217;

    ASSERT_EQ(matrix.size(), 1u);
    const auto [x, y, value] = *matrix.begin();
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 100);
    EXPECT_EQ(value, 217);
}

TEST(Matrix, SupportsArbitraryDimensions) {
    Matrix<int, 0, 3> matrix;
    matrix[-1][2][3] = 42;

    EXPECT_EQ(matrix[-1][2][3], 42);
    ASSERT_EQ(matrix.size(), 1u);
    const auto [x, y, z, value] = *matrix.begin();
    EXPECT_EQ(x, -1);
    EXPECT_EQ(y, 2);
    EXPECT_EQ(z, 3);
    EXPECT_EQ(value, 42);
}
