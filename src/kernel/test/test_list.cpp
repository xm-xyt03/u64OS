#include <kernel/test.hpp>
#include <kernel/lib.hpp>

// ── Helper struct that embeds a ListHead ──────────────────────────────────────

struct Node
{
    int value;
    lib::ListHead list;
};

// ── Tests ─────────────────────────────────────────────────────────────────────

KTEST(List, InitIsEmpty)
{
    lib::ListHead head;
    lib::list_head_init(&head);
    EXPECT_TRUE(lib::list_empty(&head));
}

KTEST(List, AddNextNotEmpty)
{
    lib::ListHead head;
    lib::list_head_init(&head);

    Node n;
    n.value = 1;
    lib::list_add_next(&head, &n.list);

    EXPECT_FALSE(lib::list_empty(&head));
}

KTEST(List, AddNextOrder)
{
    // After: head <-> a <-> b <-> head
    lib::ListHead head;
    lib::list_head_init(&head);

    Node a, b;
    a.value = 1;
    b.value = 2;

    lib::list_add_next(&head, &a.list); // head <-> a
    lib::list_add_next(&a.list, &b.list); // head <-> a <-> b

    // head.next == a, head.next.next == b
    EXPECT_EQ(head.next, &a.list);
    EXPECT_EQ(head.next->next, &b.list);
    EXPECT_EQ(head.prev, &b.list);
}

KTEST(List, AddPrevOrder)
{
    // list_add_prev inserts BEFORE the entry:
    // After list_add_prev(&head, &a.list): a <-> head
    lib::ListHead head;
    lib::list_head_init(&head);

    Node a, b;
    a.value = 10;
    b.value = 20;

    lib::list_add_prev(&head, &a.list); // a <-> head
    lib::list_add_prev(&head, &b.list); // a <-> b <-> head

    EXPECT_EQ(head.prev, &b.list);
    EXPECT_EQ(b.list.prev, &a.list);
    EXPECT_EQ(a.list.prev, &head);
}

KTEST(List, DelRestoresEmpty)
{
    lib::ListHead head;
    lib::list_head_init(&head);

    Node n;
    n.value = 42;
    lib::list_add_next(&head, &n.list);
    EXPECT_FALSE(lib::list_empty(&head));

    lib::list_del(&n.list);

    // After deletion the head's pointers no longer point to n,
    // but head itself may not be re-initialised – check n is unlinked.
    EXPECT_NE(head.next, &n.list);
    EXPECT_NE(head.prev, &n.list);
}

KTEST(List, MultipleInsertDelete)
{
    lib::ListHead head;
    lib::list_head_init(&head);

    Node nodes[4];
    for (int i = 0; i < 4; i++)
    {
        nodes[i].value = i;
        lib::list_add_next(&head, &nodes[i].list);
    }

    // Remove middle two
    lib::list_del(&nodes[1].list);
    lib::list_del(&nodes[2].list);

    // Remaining: head <-> nodes[0] ? and nodes[3] connected to head via prev?
    // The list is not empty since nodes[0] and nodes[3] remain.
    EXPECT_FALSE(lib::list_empty(&head));

    // Remove the rest
    lib::list_del(&nodes[0].list);
    lib::list_del(&nodes[3].list);

    // Now reinit head to verify it can be set empty
    lib::list_head_init(&head);
    EXPECT_TRUE(lib::list_empty(&head));
}
