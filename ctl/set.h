/* Red-black tree.
   SPDX-License-Identifier: MIT */

#ifndef T
#error "Template type T undefined for <ctl/set.h>"
#endif

// TODO emplace, extract, extract_it, merge, equal_range

#define CTL_SET
#define A JOIN(set, T)
#define B JOIN(A, node)
#ifndef C
# define C set
#endif
#define I JOIN(A, it)
#undef IT
#define IT B*

#include <ctl/ctl.h>
#include <ctl/bits/iterators.h>

typedef struct B
{
    struct B* l;
    struct B* r;
    struct B* p;
    T value;
    int color; // Red = 0, Black = 1
} B;

typedef struct A
{
    B* root;
    size_t size;
    void (*free)(T*);
    T (*copy)(T*);
    int (*compare)(T*, T*);
    int (*equal)(T*, T*);
} A;

typedef struct I
{
    B* node;
    T* ref;
    B* end;
    A* container;
} I;

static inline I
JOIN(I, iter)(A* self, B *node)
{
    static I zero;
    I iter = zero;
    iter.node = node;
    if (node)
        iter.ref = &node->value;
    iter.container = self;
    // end defaults to NULL
    return iter;
}

static inline B*
JOIN(B, min)(B* node)
{
    if (node)
        while(node->l)
            node = node->l;
    return node;
}

static inline B*
JOIN(A, first)(A* self)
{
    return JOIN(B, min)(self->root);
}

static inline I
JOIN(A, begin)(A* self)
{
    return JOIN(I, iter)(self, JOIN(B, min)(self->root));
}

static inline I
JOIN(A, end)(A* self)
{
    return JOIN(I, iter)(self, NULL);
}

static inline B*
JOIN(B, max)(B* node)
{
    while(node->r)
        node = node->r;
    return node;
}

static inline B*
JOIN(A, last)(A* self)
{
    return JOIN(B, max)(self->root);
}

static inline B*
JOIN(B, next)(B* node)
{
    if(node->r)
    {
        node = node->r;
        while(node->l)
            node = node->l;
    }
    else
    {
        B* parent = node->p;
        while(parent && node == parent->r)
        {
            node = parent;
            parent = parent->p;
        }
        node = parent;
    }
    return node;
}

static inline T*
JOIN(I, ref)(I* iter)
{
    return &iter->node->value;
}

static inline int
JOIN(I, done)(I* iter)
{
    return iter->node == iter->end;
}

static inline int
JOIN(I, is_end)(I* iter, I* last)
{
    return iter->node == last->node;
}

static inline I*
JOIN(I, next)(I* iter)
{
    iter->node = JOIN(B, next)(iter->node);
    iter->ref = iter->node ? &iter->node->value : NULL;
    return iter;
}

static inline I*
JOIN(I, advance)(I* iter, long i)
{
    A* a = iter->container;
    if (i < 0)
    {
        if ((size_t)-i > a->size)
            return NULL;
        i = a->size + i;
        iter->node = iter->container->root;
    }
    for(long j = 0; iter->node != NULL && j < i; j++)
        iter->node = JOIN(B, next)(iter->node);
    iter->ref = &iter->node->value;
    return iter;
}

static inline long
JOIN(I, distance)(I* iter, I* other)
{
    long d = 0;
    if (iter == other || iter->node == other->node)
        return 0;
    B* n = iter->node;
    for(; n != NULL && n != other->node; d++)
        n = JOIN(B, next)(n);
    if (n == other->node)
        return d;
    // other before iter, negative result
    d = (long)other->container->size;
    n = other->node;
    for(; n != NULL && n != iter->node; d--)
        n = JOIN(B, next)(n);
    return n ? -d : LONG_MAX;
}

// set first and last ranges
static inline void
JOIN(I, range)(I* iter, I* last)
{
    last->end = iter->end = last->node;
}

static inline A JOIN(A, copy)(A* self);

#include <ctl/bits/container.h>

static inline A
JOIN(A, init)(int _compare(T*, T*))
{
    static A zero;
    A self = zero;
    self.compare = _compare;
#ifdef POD
    self.copy = JOIN(A, implicit_copy);
    _JOIN(A, _set_default_methods)(&self);
#else
    self.free = JOIN(T, free);
    self.copy = JOIN(T, copy);
#endif
    return self;
}

static inline void
JOIN(A, free_node)(A* self, B* node)
{
#ifndef POD
    if(self->free)
        self->free(&node->value);
#else
    (void) self;
#endif
    free(node);
}

static inline int
JOIN(B, color)(B* node)
{
    return node ? node->color : 1;
}

static inline int
JOIN(B, is_black)(B* node)
{
    return JOIN(B, color)(node) == 1;
}

static inline int
JOIN(B, is_red)(B* node)
{
    return JOIN(B, color)(node) == 0;
}

static inline B*
JOIN(B, grandfather)(B* node)
{
    return node->p->p;
}

static inline B*
JOIN(B, sibling)(B* node)
{
    if(node == node->p->l)
        return node->p->r;
    else
        return node->p->l;
}

static inline B*
JOIN(B, uncle)(B* node)
{
    return JOIN(B, sibling)(node->p);
}

static inline B*
JOIN(B, init)(T key, int color)
{
    B* node = (B*) malloc(sizeof(B));
    node->value = key;
    node->color = color;
    node->l = node->r = node->p = NULL;
    return node;
}

static inline B*
JOIN(A, find_node)(A* self, T key)
{
    B* node = self->root;
    while(node)
    {
        int diff = self->compare(&key, &node->value);
        // Don't rely on a valid 3-way compare. can be just a simple >
        if(diff == 0)
        {
            if (self->equal)
            {
                if (self->equal(&key, &node->value))
                    return node;
                else
                    node = node->r;
            }
            else
                return node;
        }
        else if(diff < 0)
            node = node->l;
        else
            node = node->r;
    }
    return NULL;
}

static inline I
JOIN(A, find)(A* self, T key)
{
    B* node = JOIN(A, find_node)(self, key);
    if (node)
        return JOIN(I, iter)(self, node);
    else
        return JOIN(A, end)(self);
}

static inline int
JOIN(A, count)(A* self, T key)
{
    B* node = JOIN(A, find_node)(self, key);
#ifndef POD
    if(self->free)
        self->free(&key);
#endif
    return node ? 1 : 0;
}

static inline int
JOIN(A, contains)(A* self, T key)
{
    return JOIN(A, count)(self, key) == 1;
}

static inline void
JOIN(B, replace)(A* self, B* a, B* b)
{
    if(a->p)
    {
        if(a == a->p->l)
            a->p->l = b;
        else
            a->p->r = b;
    }
    else
        self->root = b;
    if(b)
        b->p = a->p;
}

#ifdef USE_INTERNAL_VERIFY

#include <assert.h>

static inline void
JOIN(B, verify_property_1)(B* node)
{
    assert(JOIN(B, is_red)(node) || JOIN(B, is_black)(node));
    if(node)
    {
        JOIN(B, verify_property_1)(node->l);
        JOIN(B, verify_property_1)(node->r);
    }
}

static inline void
JOIN(B, verify_property_2)(B* node)
{
    assert(JOIN(B, is_black)(node));
}

static inline void
JOIN(B, verify_property_4)(B* node)
{
    if(JOIN(B, is_red)(node))
    {
        assert(JOIN(B, is_black)(node->l));
        assert(JOIN(B, is_black)(node->r));
        assert(JOIN(B, is_black)(node->p));
    }
    if(node)
    {
        JOIN(B, verify_property_4)(node->l);
        JOIN(B, verify_property_4)(node->r);
    }
}

static inline void
JOIN(B, count_black)(B* node, int nodes, int* in_path)
{
    if(JOIN(B, is_black)(node))
        nodes++;
    if(node)
    {
        JOIN(B, count_black)(node->l, nodes, in_path);
        JOIN(B, count_black)(node->r, nodes, in_path);
    }
    else
    {
        if(*in_path == -1)
            *in_path = nodes;
        else
            assert(nodes == *in_path);
    }
}

static inline void
JOIN(B, verify_property_5)(B* node)
{
    int in_path = -1;
    JOIN(B, count_black)(node, 0, &in_path);
}

static inline void
JOIN(A, verify)(A* self)
{
    JOIN(B, verify_property_1)(self->root); // Property 1: Each node is either red or black.
    JOIN(B, verify_property_2)(self->root); // Property 2: The root node is black.
    /* Implicit */                          // Property 3: Leaves are colored black
    JOIN(B, verify_property_4)(self->root); // Property 4: Every red node has two black nodes.
    JOIN(B, verify_property_5)(self->root); // Property 5: All paths from a node have the same number of black nodes.
}

#endif

static inline void
JOIN(A, rotate_l)(A* self, B* node)
{
    B* r = node->r;
    JOIN(B, replace)(self, node, r);
    node->r = r->l;
    if(r->l)
        r->l->p = node;
    r->l = node;
    node->p = r;
}

static inline void
JOIN(A, rotate_r)(A* self, B* node)
{
    B* l = node->l;
    JOIN(B, replace)(self, node, l);
    node->l = l->r;
    if(l->r)
        l->r->p = node;
    l->r = node;
    node->p = l;
}

static inline void
JOIN(A, insert_1)(A*, B*),
JOIN(A, insert_2)(A*, B*),
JOIN(A, insert_3)(A*, B*),
JOIN(A, insert_4)(A*, B*),
JOIN(A, insert_5)(A*, B*);

// TODO bulk insert of sorted vector
static inline B*
JOIN(A, insert)(A* self, T key)
{
    B* insert = JOIN(B, init)(key, 0);
    if(self->root)
    {
        B* node = self->root;
        while(1)
        {
            int diff = self->compare(&key, &node->value);
            if(diff == 0) // equal: replace
            {
                JOIN(A, free_node)(self, insert);
                return node;
            }
            else if(diff < 0) // lower
            {
                if(node->l)
                    node = node->l;
                else
                {
                    node->l = insert;
                    break;
                }
            }
            else // greater
            {
                if(node->r)
                    node = node->r;
                else
                {
                    node->r = insert;
                    break;
                }
            }
        }
        insert->p = node;
    }
    else
        self->root = insert;
    JOIN(A, insert_1)(self, insert);
    self->size++;
#ifdef USE_INTERNAL_VERIFY
    JOIN(A, verify)(self);
#endif
    return insert;
}

static inline void
JOIN(A, insert_1)(A* self, B* node)
{
    if(node->p)
        JOIN(A, insert_2)(self, node);
    else
        node->color = 1;
}

static inline void
JOIN(A, insert_2)(A* self, B* node)
{
    if(JOIN(B, is_black)(node->p))
        return;
    else
       JOIN(A, insert_3)(self, node);
}

static inline void
JOIN(A, insert_3)(A* self, B* node)
{
    if(JOIN(B, is_red)(JOIN(B, uncle)(node)))
    {
        node->p->color = 1;
        JOIN(B, uncle)(node)->color = 1;
        JOIN(B, grandfather)(node)->color = 0;
        JOIN(A, insert_1)(self, JOIN(B, grandfather)(node));
    }
    else
        JOIN(A, insert_4)(self, node);
}

static inline void
JOIN(A, insert_4)(A* self, B* node)
{
    if(node == node->p->r && node->p == JOIN(B, grandfather)(node)->l)
    {
        JOIN(A, rotate_l)(self, node->p);
        node = node->l;
    }
    else
    if(node == node->p->l && node->p == JOIN(B, grandfather)(node)->r)
    {
        JOIN(A, rotate_r)(self, node->p);
        node = node->r;
    }
    JOIN(A, insert_5)(self, node);
}

static inline void
JOIN(A, insert_5)(A* self, B* node)
{
    node->p->color = 1;
    JOIN(B, grandfather)(node)->color = 0;
    if(node == node->p->l && node->p == JOIN(B, grandfather)(node)->l)
        JOIN(A, rotate_r)(self, JOIN(B, grandfather)(node));
    else
        JOIN(A, rotate_l)(self, JOIN(B, grandfather)(node));
}

static inline void
JOIN(A, erase_1)(A*, B*),
JOIN(A, erase_2)(A*, B*),
JOIN(A, erase_3)(A*, B*),
JOIN(A, erase_4)(A*, B*),
JOIN(A, erase_5)(A*, B*),
JOIN(A, erase_6)(A*, B*);

static inline void
JOIN(A, erase_node)(A* self, B* node)
{
    if(node->l && node->r)
    {
        B* pred = JOIN(B, max)(node->l);
        SWAP(T, &node->value, &pred->value);
        node = pred;
    }
    B* child = node->r ? node->r : node->l;
    if(JOIN(B, is_black)(node))
    {
        node->color = JOIN(B, color)(child);
        JOIN(A, erase_1)(self, node);
    }
    JOIN(B, replace)(self, node, child);
    if(!node->p && child)
        child->color = 1;
    JOIN(A, free_node)(self, node);
    self->size--;
#ifdef USE_INTERNAL_VERIFY
    JOIN(A, verify)(self);
#endif
}

static inline void
JOIN(A, erase_it)(I* it)
{
    B* node = it->node;
    if(node)
        JOIN(A, erase_node)(it->container, node);
}

static inline void
JOIN(A, erase_range)(I* from, I* to)
{
    if(!JOIN(I, done)(from))
    {
        // TODO: check if clear would be faster (from==begin && to==end)
        B* node = from->node;
        while(node != to->node)
        {
            B* next = JOIN(B, next)(node);
            JOIN(A, erase_node)(from->container, node);
            node = next;
        }
    }
}

static inline void
JOIN(A, erase)(A* self, T key)
{
    B* node = JOIN(A, find_node)(self, key);
    if(node)
        JOIN(A, erase_node)(self, node);
}

static inline void
JOIN(A, erase_1)(A* self, B* node)
{
    if(node->p)
        JOIN(A, erase_2)(self, node);
}

static inline void
JOIN(A, erase_2)(A* self, B* node)
{
    if(JOIN(B, is_red)(JOIN(B, sibling)(node)))
    {
        node->p->color = 0;
        JOIN(B, sibling)(node)->color = 1;
        if(node == node->p->l)
            JOIN(A, rotate_l)(self, node->p);
        else
            JOIN(A, rotate_r)(self, node->p);
    }
    JOIN(A, erase_3)(self, node);
}

static inline void
JOIN(A, erase_3)(A* self, B* node)
{
    if(JOIN(B, is_black)(node->p)
    && JOIN(B, is_black)(JOIN(B, sibling)(node))
    && JOIN(B, is_black)(JOIN(B, sibling)(node)->l)
    && JOIN(B, is_black)(JOIN(B, sibling)(node)->r))
    {
        JOIN(B, sibling)(node)->color = 0;
        JOIN(A, erase_1)(self, node->p);
    }
    else
        JOIN(A, erase_4)(self, node);
}

static inline void
JOIN(A, erase_4)(A* self, B* node)
{
    if(JOIN(B, is_red)(node->p)
    && JOIN(B, is_black)(JOIN(B, sibling)(node))
    && JOIN(B, is_black)(JOIN(B, sibling)(node)->l)
    && JOIN(B, is_black)(JOIN(B, sibling)(node)->r))
    {
        JOIN(B, sibling)(node)->color = 0;
        node->p->color = 1;
    }
    else
        JOIN(A, erase_5)(self, node);
}

static inline void
JOIN(A, erase_5)(A* self, B* node)
{
    if(node == node->p->l
    && JOIN(B, is_black)(JOIN(B, sibling)(node))
    && JOIN(B, is_red)(JOIN(B, sibling)(node)->l)
    && JOIN(B, is_black)(JOIN(B, sibling)(node)->r))
    {
        JOIN(B, sibling)(node)->color = 0;
        JOIN(B, sibling)(node)->l->color = 1;
        JOIN(A, rotate_r)(self, JOIN(B, sibling)(node));
    }
    else
    if(node == node->p->r
    && JOIN(B, is_black)(JOIN(B, sibling)(node))
    && JOIN(B, is_red)(JOIN(B, sibling)(node)->r)
    && JOIN(B, is_black)(JOIN(B, sibling)(node)->l))
    {
        JOIN(B, sibling)(node)->color = 0;
        JOIN(B, sibling)(node)->r->color = 1;
        JOIN(A, rotate_l)(self, JOIN(B, sibling)(node));
    }
    JOIN(A, erase_6)(self, node);
}

static inline void
JOIN(A, erase_6)(A* self, B* node)
{
    JOIN(B, sibling)(node)->color = JOIN(B, color)(node->p);
    node->p->color = 1;
    if(node == node->p->l)
    {
        JOIN(B, sibling)(node)->r->color = 1;
        JOIN(A, rotate_l)(self, node->p);
    }
    else
    {
        JOIN(B, sibling)(node)->l->color = 1;
        JOIN(A, rotate_r)(self, node->p);
    }
}

// erase without rebalancing. e.g. for clear
static inline void
JOIN(B, erase_fast)(A* self, B* node)
{
    while(node)
    {
        JOIN(B, erase_fast)(self, node->r);
        B* left = node->l;
        JOIN(A, free_node)(self, node);
        node = left;
        self->size--;
    }
}

static inline void
JOIN(A, clear)(A* self)
{
    while(!JOIN(A, empty)(self))
        JOIN(B, erase_fast)(self, self->root);
    self->root = NULL;
}

static inline void
JOIN(A, free)(A* self)
{
    JOIN(A, clear)(self);
    *self = JOIN(A, init)(self->compare);
}

static inline A
JOIN(A, copy)(A* self)
{
    A copy =  JOIN(A, init)(self->compare);
    list_foreach_ref(A, self, it)
        JOIN(A, insert)(&copy, self->copy(it.ref));
    return copy;
}

static inline void
JOIN(A, swap)(A* self, A* other)
{
    A temp = *self;
    *self = *other;
    *other = temp;
}

static inline size_t
JOIN(A, remove_if)(A* self, int (*_match)(T*))
{
    size_t erases = 0;
    B* node = JOIN(A, first)(self);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        if(_match(&node->value))
        {
            JOIN(A, erase_node)(self, node);
            erases++;
        }
        node = next;
    }
    return erases;
}

static inline size_t
JOIN(A, erase_if)(A* self, int (*_match)(T*))
{
    return JOIN(A, remove_if)(self, _match);
}

static inline void /* I, B* ?? */
JOIN(A, unique)(A* self)
{
    B* node = JOIN(A, first)(self);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        if(next && JOIN(A, _equal)(self, &node->value, &next->value))
            JOIN(A, erase_node)(self, node);
        node = next;
    }
}

static inline A
JOIN(A, intersection)(A* a, A* b)
{
    A self = JOIN(A, init)(a->compare);
    B* node = JOIN(A, first)(a);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        if(JOIN(A, find_node)(b, node->value))
            JOIN(A, insert)(&self, self.copy(&node->value));
        node = next;
    }
    return self;
}

static inline A
JOIN(A, union)(A* a, A* b)
{
    A self = JOIN(A, init)(a->compare);
    B* node = JOIN(A, first)(a);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        JOIN(A, insert)(&self, self.copy(&node->value));
        node = next;
    }
    node = JOIN(A, first)(b);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        JOIN(A, insert)(&self, self.copy(&node->value));
        node = next;
    }
    return self;
}

static inline A
JOIN(A, difference)(A* a, A* b)
{
    A self = JOIN(A, copy)(a);
    B* node = JOIN(A, first)(b);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        JOIN(A, erase)(&self, node->value);
        node = next;
    }
    return self;
}

static inline A
JOIN(A, symmetric_difference)(A* a, A* b)
{
    A self = JOIN(A, union)(a, b);
    A intersection = JOIN(A, intersection)(a, b);
    B* node = JOIN(A, first)(&intersection);
    while (node)
    {
        B* next = JOIN(B, next)(node);
        JOIN(A, erase)(&self, node->value);
        node = next;
    }
    JOIN(A, free)(&intersection);
    return self;
}

#if defined(CTL_MAP)
# include <ctl/algorithm.h>
#endif

#ifndef HOLD
#undef POD
#undef NOT_INTEGRAL
#undef T
#undef A
#undef B
#undef C
#undef I
#else
#undef HOLD
#endif
#undef CTL_SET

#ifdef USE_INTERNAL_VERIFY
#undef USE_INTERNAL_VERIFY
#endif
