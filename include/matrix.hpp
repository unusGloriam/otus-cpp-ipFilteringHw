#ifndef MATRIX_MATRIX_HPP
#define MATRIX_MATRIX_HPP

#include <map>
#include <iterator>
#include <memory>
#include <tuple>
#include <utility>


/// Represents a 2D matrix with customizable default value.
template <typename T = int, T DefaultValue = T{-1}>
class Matrix {
    using Position = std::pair<int, int>;
    using Storage = std::map<Position, T>;

    struct State {
        Storage cells;
        T default_value = DefaultValue;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();

    class Cell {
        std::shared_ptr<State> state_;
        Position position_;

    public:
        Cell(std::shared_ptr<State> state, int x, int y)
            : state_(std::move(state)), position_(x, y) {}

        Cell(const Cell&) = default;
        Cell(Cell&&) noexcept = default;
        Cell& operator=(const Cell&) = default;
        Cell& operator=(Cell&&) noexcept = default;
        ~Cell() = default;

        Cell& operator=(const T& value) {
            if (value == state_->default_value) {
                state_->cells.erase(position_);
            } else {
                state_->cells[position_] = value;
            }
            return *this;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator T() const {
            const auto found = state_->cells.find(position_);
            return found == state_->cells.end() ? state_->default_value : found->second;
        }
    };

    class Row {
        std::shared_ptr<State> state_;
        int x_;

    public:
        Row(std::shared_ptr<State> state, int x) : state_(std::move(state)), x_(x) {}
        Cell operator[](int y) { return Cell(state_, x_, y); }
    };

public:
    using value_type = std::tuple<int, int, T>;

    explicit Matrix(T default_value = DefaultValue) {
        state_->default_value = default_value;
    }

    Matrix(const Matrix& other) : state_(std::make_shared<State>(*other.state_)) {}
    Matrix& operator=(const Matrix& other) {
        if (this != &other) state_ = std::make_shared<State>(*other.state_);
        return *this;
    }
    Matrix(Matrix&&) noexcept = default;
    Matrix& operator=(Matrix&&) noexcept = default;
    ~Matrix() = default;

    Row operator[](int x) { return Row(state_, x); }

    [[nodiscard]] std::size_t size() const { return state_->cells.size(); }

    class const_iterator {
        Storage::const_iterator current_;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::tuple<int, int, T>;
        using difference_type = std::ptrdiff_t;

        explicit const_iterator(Storage::const_iterator current) : current_(current) {}
        value_type operator*() const { return {current_->first.first, current_->first.second, current_->second}; }
        const_iterator& operator++() { ++current_; return *this; }
        bool operator==(const const_iterator& other) const { return current_ == other.current_; }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
    };

    const_iterator begin() const { return const_iterator(state_->cells.cbegin()); }
    const_iterator end() const { return const_iterator(state_->cells.cend()); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
};

#endif
