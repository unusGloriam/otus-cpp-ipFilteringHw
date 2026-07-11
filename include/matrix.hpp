#ifndef MATRIX_MATRIX_HPP
#define MATRIX_MATRIX_HPP

#include <array>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <tuple>
#include <utility>


/// N-мерная матрица.
/// T - Тип элементов матрицы.
/// DefaultValue - Значение по умолчанию.
/// Dimensions - Количество измерений.
template<typename T = int, T DefaultValue = T{-1}, std::size_t Dimensions = 2>
class Matrix {
    static_assert(Dimensions > 0, "Matrix must have at least one dimension");

    using Position = std::array<size_t, Dimensions>;
    using Storage = std::map<Position, T>;

    struct State {
        Storage cells;
        T default_value = DefaultValue;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();

    template<std::size_t>
    using coordinate_type = size_t;

    template<typename IndexSequence>
    struct tuple_type;

    template<std::size_t... Indexes>
    struct tuple_type<std::index_sequence<Indexes...>> {
        using type = std::tuple<coordinate_type<Indexes>..., T>;
    };

    template<typename IndexSequence>
    using tuple_type_t = tuple_type<IndexSequence>::type;

    template<std::size_t... Indexes>
    static auto make_value(const Position &position, const T &value,
                           std::index_sequence<Indexes...>)
        -> tuple_type_t<std::index_sequence<Indexes...>> {
        return {position[Indexes]..., value};
    }

    class Cell {
        std::shared_ptr<State> state_;
        Position position_{};

    public:
        Cell(std::shared_ptr<State> state, Position position)
            : state_(std::move(state)), position_(position) {
        }

        Cell(const Cell &) = default;

        Cell(Cell &&) noexcept = default;

        Cell &operator=(const Cell &) = default;

        Cell &operator=(Cell &&) noexcept = default;

        ~Cell() = default;

        Cell &operator=(const T &value) {
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

    template<std::size_t Remaining>
    class IndexProxy {
        std::shared_ptr<State> state_;
        Position position_;

    public:
        IndexProxy(std::shared_ptr<State> state, Position position)
            : state_(std::move(state)), position_(position) {
        }

        auto operator[](int index) {
            Position next = position_;
            next[Dimensions - Remaining] = index;
            if constexpr (Remaining == 1) {
                return Cell(state_, next);
            } else {
                return IndexProxy<Remaining - 1>(state_, next);
            }
        }
    };

public:
    using value_type = tuple_type_t<std::make_index_sequence<Dimensions>>;

    explicit Matrix(T default_value = DefaultValue) {
        state_->default_value = default_value;
    }

    Matrix(const Matrix &other) : state_(std::make_shared<State>(*other.state_)) {
    }

    Matrix &operator=(const Matrix &other) {
        if (this != &other) state_ = std::make_shared<State>(*other.state_);
        return *this;
    }

    Matrix(Matrix &&) noexcept = default;

    Matrix &operator=(Matrix &&) noexcept = default;

    ~Matrix() = default;

    auto operator[](int index) {
        Position position{};
        position[0] = index;
        if constexpr (Dimensions == 1) {
            return Cell(state_, position);
        } else {
            return IndexProxy<Dimensions - 1>(state_, position);
        }
    }

    [[nodiscard]] std::size_t size() const { return state_->cells.size(); }

    class iterator {
        Storage::iterator current_;

    public:
        using iterator_category = std::forward_iterator_tag;
        // ReSharper disable once CppRedundantQualifier ошибка clang-tidy
        using value_type = Matrix::value_type;
        using difference_type = std::ptrdiff_t;

        explicit iterator(Storage::iterator current) : current_(current) {
        }

        value_type operator*() const {
            return make_value(current_->first, current_->second,
                              std::make_index_sequence<Dimensions>{});
        }

        iterator &operator++() {
            ++current_;
            return *this;
        }

        bool operator==(const iterator &other) const { return current_ == other.current_; }
        bool operator!=(const iterator &other) const { return !(*this == other); }
    };

    iterator begin() { return iterator(state_->cells.begin()); }
    iterator end() { return iterator(state_->cells.end()); }
};

#endif
