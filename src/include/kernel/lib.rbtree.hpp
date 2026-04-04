#ifndef KERNEL_LIB_RBTREE_HPP
#define KERNEL_LIB_RBTREE_HPP

#include <kernel/base.hpp>
#include <u64OS/compiler.h>

namespace lib
{
    /*
     * Minimal intrusive red-black tree.
     *
     * Usage:
     *   1. Embed an RbNode inside your struct.
     *   2. Provide a Comparator: int compare(const RbNode *a, const RbNode *b)
     *      returning <0, 0, or >0.
     *   3. Use RbTree<Comparator> to insert/find/remove nodes.
     *   4. Use container_of() to recover the enclosing struct from an RbNode*.
     */

    enum RbColor : base::uint8_t
    {
        RB_RED = 0,
        RB_BLACK = 1,
    };

    struct RbNode
    {
        RbNode *parent;
        RbNode *left;
        RbNode *right;
        RbColor color;

        constexpr RbNode() : parent(nullptr), left(nullptr), right(nullptr), color(RB_RED) {}
    };

    namespace rbtree_internal
    {
        static __always_inline auto rb_set_parent(RbNode *n, RbNode *p) -> void
        {
            n->parent = p;
        }

        static __always_inline auto rb_is_red(RbNode *n) -> bool
        {
            return n && n->color == RB_RED;
        }

        static __always_inline auto rb_is_black(RbNode *n) -> bool
        {
            return !n || n->color == RB_BLACK;
        }

        static __always_inline auto rb_set_black(RbNode *n) -> void
        {
            if (n)
                n->color = RB_BLACK;
        }

        /* Replace child pointer in parent */
        static __always_inline auto rb_replace_child(RbNode *&root, RbNode *parent,
                                                      RbNode *old_child, RbNode *new_child) -> void
        {
            if (!parent)
                root = new_child;
            else if (parent->left == old_child)
                parent->left = new_child;
            else
                parent->right = new_child;

            if (new_child)
                new_child->parent = parent;
        }

        /* Left rotation around node n */
        static auto rb_rotate_left(RbNode *&root, RbNode *n) -> void
        {
            RbNode *r = n->right;
            RbNode *p = n->parent;

            n->right = r->left;
            if (r->left)
                r->left->parent = n;

            r->left = n;
            n->parent = r;

            rb_replace_child(root, p, n, r);
        }

        /* Right rotation around node n */
        static auto rb_rotate_right(RbNode *&root, RbNode *n) -> void
        {
            RbNode *l = n->left;
            RbNode *p = n->parent;

            n->left = l->right;
            if (l->right)
                l->right->parent = n;

            l->right = n;
            n->parent = l;

            rb_replace_child(root, p, n, l);
        }

        /* Fix tree after insertion of node n */
        static auto rb_insert_fixup(RbNode *&root, RbNode *n) -> void
        {
            while (rb_is_red(n->parent))
            {
                RbNode *parent = n->parent;
                RbNode *grandp = parent->parent;

                if (!grandp)
                    break;

                if (parent == grandp->left)
                {
                    RbNode *uncle = grandp->right;
                    if (rb_is_red(uncle))
                    {
                        /* Case 1: uncle is red — recolor */
                        parent->color = RB_BLACK;
                        uncle->color = RB_BLACK;
                        grandp->color = RB_RED;
                        n = grandp;
                    }
                    else
                    {
                        if (n == parent->right)
                        {
                            /* Case 2: n is right child — rotate left */
                            rb_rotate_left(root, parent);
                            n = parent;
                            parent = n->parent;
                        }
                        /* Case 3: n is left child — rotate right */
                        parent->color = RB_BLACK;
                        grandp->color = RB_RED;
                        rb_rotate_right(root, grandp);
                    }
                }
                else
                {
                    RbNode *uncle = grandp->left;
                    if (rb_is_red(uncle))
                    {
                        parent->color = RB_BLACK;
                        uncle->color = RB_BLACK;
                        grandp->color = RB_RED;
                        n = grandp;
                    }
                    else
                    {
                        if (n == parent->left)
                        {
                            rb_rotate_right(root, parent);
                            n = parent;
                            parent = n->parent;
                        }
                        parent->color = RB_BLACK;
                        grandp->color = RB_RED;
                        rb_rotate_left(root, grandp);
                    }
                }
            }
            root->color = RB_BLACK;
        }

        /* Fix tree after removal (double-black at n with parent p) */
        static auto rb_erase_fixup(RbNode *&root, RbNode *n, RbNode *parent) -> void
        {
            while (n != root && rb_is_black(n))
            {
                if (n == parent->left)
                {
                    RbNode *sibling = parent->right;

                    if (rb_is_red(sibling))
                    {
                        sibling->color = RB_BLACK;
                        parent->color = RB_RED;
                        rb_rotate_left(root, parent);
                        sibling = parent->right;
                    }

                    if (rb_is_black(sibling->left) && rb_is_black(sibling->right))
                    {
                        if (sibling)
                            sibling->color = RB_RED;
                        n = parent;
                        parent = n->parent;
                    }
                    else
                    {
                        if (rb_is_black(sibling->right))
                        {
                            rb_set_black(sibling->left);
                            sibling->color = RB_RED;
                            rb_rotate_right(root, sibling);
                            sibling = parent->right;
                        }
                        sibling->color = parent->color;
                        parent->color = RB_BLACK;
                        rb_set_black(sibling->right);
                        rb_rotate_left(root, parent);
                        n = root;
                    }
                }
                else
                {
                    RbNode *sibling = parent->left;

                    if (rb_is_red(sibling))
                    {
                        sibling->color = RB_BLACK;
                        parent->color = RB_RED;
                        rb_rotate_right(root, parent);
                        sibling = parent->left;
                    }

                    if (rb_is_black(sibling->right) && rb_is_black(sibling->left))
                    {
                        if (sibling)
                            sibling->color = RB_RED;
                        n = parent;
                        parent = n->parent;
                    }
                    else
                    {
                        if (rb_is_black(sibling->left))
                        {
                            rb_set_black(sibling->right);
                            sibling->color = RB_RED;
                            rb_rotate_left(root, sibling);
                            sibling = parent->left;
                        }
                        sibling->color = parent->color;
                        parent->color = RB_BLACK;
                        rb_set_black(sibling->left);
                        rb_rotate_right(root, parent);
                        n = root;
                    }
                }
            }
            rb_set_black(n);
        }

        /* Remove node n from the tree */
        static auto rb_erase(RbNode *&root, RbNode *n) -> void
        {
            RbNode *child, *parent;
            RbColor color;

            if (!n->left)
            {
                child = n->right;
                parent = n->parent;
                color = n->color;
                rb_replace_child(root, parent, n, child);
            }
            else if (!n->right)
            {
                child = n->left;
                parent = n->parent;
                color = n->color;
                rb_replace_child(root, parent, n, child);
            }
            else
            {
                /* Find in-order successor (leftmost node in right subtree) */
                RbNode *succ = n->right;
                while (succ->left)
                    succ = succ->left;

                child = succ->right;
                parent = succ->parent;
                color = succ->color;

                if (parent == n)
                    parent = succ;
                else
                {
                    if (child)
                        child->parent = parent;
                    parent->left = child;
                    succ->right = n->right;
                    n->right->parent = succ;
                }

                rb_replace_child(root, n->parent, n, succ);
                succ->left = n->left;
                n->left->parent = succ;
                succ->color = n->color;
            }

            if (color == RB_BLACK)
                rb_erase_fixup(root, child, parent);
        }

    } /* namespace rbtree_internal */

    /*
     * RbTree<Cmp> — a red-black tree parameterized by a comparator type.
     *
     * Cmp must provide:
     *   static int compare(const RbNode *a, const RbNode *b);
     * returning negative if a < b, zero if equal, positive if a > b.
     */
    template <typename Cmp>
    class RbTree
    {
    public:
        RbTree() : root_(nullptr) {}

        /* Insert node n. No duplicates are allowed; caller must ensure uniqueness. */
        auto Insert(RbNode *n) -> void
        {
            n->left = nullptr;
            n->right = nullptr;
            n->color = RB_RED;
            n->parent = nullptr;

            RbNode **link = &root_;
            RbNode *parent = nullptr;

            while (*link)
            {
                parent = *link;
                int cmp = Cmp::compare(n, parent);
                if (cmp < 0)
                    link = &parent->left;
                else
                    link = &parent->right;
            }

            *link = n;
            n->parent = parent;

            rbtree_internal::rb_insert_fixup(root_, n);
        }

        /* Find the node for which Cmp::compare(node, key) == 0. */
        template <typename Key>
        auto Find(const Key *key) const -> RbNode *
        {
            RbNode *n = root_;
            while (n)
            {
                int cmp = Cmp::compare_key(n, key);
                if (cmp == 0)
                    return n;
                else if (cmp > 0)
                    n = n->left;
                else
                    n = n->right;
            }
            return nullptr;
        }

        /* Remove node n from the tree. */
        auto Remove(RbNode *n) -> void
        {
            rbtree_internal::rb_erase(root_, n);
            n->parent = nullptr;
            n->left = nullptr;
            n->right = nullptr;
        }

        auto Empty() const -> bool { return root_ == nullptr; }

        auto Root() const -> RbNode * { return root_; }

    private:
        RbNode *root_;
    };

} /* namespace lib */

#endif /* !KERNEL_LIB_RBTREE_HPP */
