//
// Created by miralius on 03.06.2026.
//

#ifndef ALLOCATOR_CUSTOM_ALLOCATOR_H
#define ALLOCATOR_CUSTOM_ALLOCATOR_H

template <typename A, std::size_t N>
struct fixed_allocator {
    using value_type = A;

    template <typename B>
    struct rebind {
        using other = fixed_allocator<B, N>;
    };

    void deallocate(A*, std::size_t) {
    }
};

#endif //ALLOCATOR_CUSTOM_ALLOCATOR_H
