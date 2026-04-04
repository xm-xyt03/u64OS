#ifndef KERNEL_LIB_CONTAINER_HPP
#define KERNEL_LIB_CONTAINER_HPP

#include <kernel/base.hpp>

#include <u64OS/compiler.h>

namespace lib
{
    template <typename ContainerType, typename MemberType>
    auto __always_inline offset_of(const MemberType ContainerType::*member) -> base::size_t
    {
        return (base::size_t)&(((ContainerType *)nullptr)->*member);
    }

    template <typename ContainerType, typename MemberPtrType, typename MemberType>
    auto __always_inline container_of(MemberPtrType *member_ptr, const MemberType ContainerType::*member) -> ContainerType *
    {
        return (ContainerType *)((base::size_t)member_ptr - offset_of<ContainerType>(member));
    }
}

#endif // !KERNEL_LIB_CONTAINER_HPP