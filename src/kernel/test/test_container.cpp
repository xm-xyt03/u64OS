#include <kernel/test.hpp>
#include <kernel/lib.hpp>

// ── Test structs ──────────────────────────────────────────────────────────────

struct Outer
{
    int x;
    int y;
    lib::ListHead node;
    int z;
};

struct Simple
{
    char a;
    int b;
    long c;
};

// ── Tests ─────────────────────────────────────────────────────────────────────

KTEST(Container, OffsetOfFirstMember)
{
    // First member of a struct must be at offset 0
    base::size_t off = lib::offset_of(&Outer::x);
    EXPECT_EQ(off, static_cast<base::size_t>(0));
}

KTEST(Container, OffsetOfNodeMember)
{
    // The 'node' field in Outer sits after two ints (8 bytes on x86-64)
    base::size_t off = lib::offset_of(&Outer::node);
    EXPECT_EQ(off, static_cast<base::size_t>(sizeof(int) * 2));
}

KTEST(Container, ContainerOfRoundtrip)
{
    Outer obj;
    obj.x = 7;
    obj.y = 13;
    obj.z = 99;

    lib::ListHead *node_ptr = &obj.node;

    Outer *recovered = lib::container_of(node_ptr, &Outer::node);
    EXPECT_EQ(recovered, &obj);
    EXPECT_EQ(recovered->x, 7);
    EXPECT_EQ(recovered->z, 99);
}

KTEST(Container, ContainerOfSimple)
{
    Simple s;
    s.a = 'A';
    s.b = 42;
    s.c = 1234567L;

    long *c_ptr = &s.c;
    Simple *recovered = lib::container_of(c_ptr, &Simple::c);

    EXPECT_EQ(recovered, &s);
    EXPECT_EQ(recovered->b, 42);
}

KTEST(Container, OffsetConsistency)
{
    // offset_of(node) + sizeof(ListHead) must not exceed offset_of(z)
    base::size_t off_node = lib::offset_of(&Outer::node);
    base::size_t off_z = lib::offset_of(&Outer::z);
    EXPECT_GE(off_z, off_node + sizeof(lib::ListHead));
}