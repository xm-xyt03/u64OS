#ifndef KERNEL_LIB_LIST_HPP
#define KERNEL_LIB_LIST_HPP

#include <kernel/lib.container.hpp>

#include <u64OS/compiler.h>

namespace lib
{
    struct ListHead
    {
        ListHead *prev, *next;
    };

    template <typename HeadType>
    auto __always_inline list_head_init(HeadType head) -> void
    {
        head->prev = head;
        head->next = head;
    }

    template <typename NodeType, typename PrevType, typename NextType>
    auto __always_inline list_add(NodeType node, PrevType prev, NextType next) -> void
    {
        node->prev = prev;
        prev->next = node;
        node->next = next;
        next->prev = node;
    }

    auto __always_inline list_add_prev(ListHead *entry, ListHead *prev) -> void
    {
        entry->prev->next = prev;
        prev->prev = entry->prev;
        prev->next = entry;
        entry->prev = prev;
    }

    auto __always_inline list_add_next(ListHead *entry, ListHead *next)
    {
        entry->next->prev = next;
        next->prev = entry;
        next->next = entry->next;
        entry->next = next;
    }

    template <typename NodeType>
    auto __always_inline list_del(NodeType node) -> void
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    template <typename EntryType>
    auto __always_inline list_empty(EntryType entry) -> bool
    {
        return (entry->next == entry) && (entry->prev == entry);
    }

    template <typename ListPtrType, typename ContainerType, typename MemberType>
    auto __always_inline list_entry(ListPtrType *ptr, const MemberType ContainerType::*member) -> ContainerType *
    {
        return container_of(ptr, member);
    }

#define list_head_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)
#define list_next_entry(ptr, member) container_of(ptr->member.next, typeof(*ptr), member)
#define list_entry_is_head(ptr, head, member) ((&ptr->member) == (head))

#define list_head_for_each_entry(pos, head, member)               \
    for (pos = list_head_first_entry(head, typeof(*pos), member); \
         list_entry_is_head(pos, head, member);                   \
         pos = list_next_entry(pos, member))

}

#endif // !KERNEL_LIB_LIST_HPP