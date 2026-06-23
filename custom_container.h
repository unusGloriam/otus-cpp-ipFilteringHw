//
// Created by miralius on 04.06.2026.
//

#ifndef ALLOCATOR_CUSTOM_CONTAINER_H
#define ALLOCATOR_CUSTOM_CONTAINER_H

template<typename T, typename Allocator = std::allocator<T> >
class custom_list {
    using pointer = T *;

    struct Node {
        T value;
        Node *next;
    };

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;

public:
    custom_list() : m_allocator(), m_head(nullptr), m_tail(nullptr) {
    }

    void push_back(const T &value) {
        Node *new_node = m_allocator.allocate(1);
        std::allocator_traits<NodeAllocator>::construct(m_allocator, new_node, Node{value, nullptr});

        if (!m_head) {
            m_head = new_node;
            m_tail = new_node;
        } else {
            m_tail->next = new_node;
            m_tail = new_node;
        }
    }

    struct forward_iterator {
        Node *curr;
        T &operator*() { return curr->value; }

        forward_iterator &operator++() {
            curr = curr->next;
            return *this;
        }

        bool operator!=(const forward_iterator &other) const { return curr != other.curr; }
    };

    forward_iterator begin() { return {m_head}; }
    forward_iterator end() { return {nullptr}; }

private:
    NodeAllocator m_allocator;
    Node *m_head;
    Node *m_tail;
};
#endif //ALLOCATOR_CUSTOM_CONTAINER_H
