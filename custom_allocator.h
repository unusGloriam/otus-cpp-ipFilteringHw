//
// Created by miralius on 03.06.2026.
//

#ifndef ALLOCATOR_CUSTOM_ALLOCATOR_H
#define ALLOCATOR_CUSTOM_ALLOCATOR_H

#include <array>

template<typename T, std::size_t N>
struct fixed_allocator {
    using value_type = T;
    using pointer = T *;
    using size_type = std::size_t;

    template<typename U>
    struct rebind {
        using other = fixed_allocator<U, N>;
    };

    void deallocate(pointer, size_type) {
    }

    pointer allocate(size_type n) {
        if (n > N) {
            throw std::bad_alloc();
        }

        if (m_allocated_offset + n > N) {
            throw std::bad_alloc();
        }

        pointer allocated = std::addressof(m_storage[m_allocated_offset]);
        m_allocated_offset += n;
        return allocated;
    }

private:
    std::array<T, N> m_storage;
    size_type m_allocated_offset = 0;
};

#endif //ALLOCATOR_CUSTOM_ALLOCATOR_H
