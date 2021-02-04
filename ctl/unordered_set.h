/* Unordered set as hashtable.
   This closed, linked list hashing has the advantage of keeping pointers
   into the set valid.
   The faster open addressing moves pointers. Maybe add another class for open
   hashes (hmap, hashtable, open_hashtable, ohash).

   SPDX-License-Identifier: MIT

   Tunable policies:

Growth policies:
  - CTL_USET_GROWTH_PRIMED:  slower but more secure. uses all hash
                             bits. (default)
  - CTL_USET_GROWTH_POWER2:  faster, but less secure. uses only some left-most
                             bits. not recommended with inet access.

- CTL_USET_CACHED_HASH:      store the hash in the bucket. faster find when
                             unsuccesful (eg on high load factor), but needs a bit more space.

Planned:

- CTL_USET_MOVE_TO_FRONT moves a bucket in a chain not at the top
position to the top in each access, such as find and contains, not only insert.

- CTL_USET_GROWTH_FACTOR defaults to 2.0 for CTL_USET_GROWTH_POWER2 and
`1.618` for CTL_USET_GROWTH_PRIMED.

*/
#ifndef T
# error "Template type T undefined for <unordered_set.h>"
#endif

#ifdef CTL_USET_GROWTH_PRIMED // the default
# undef CTL_USET_GROWTH_POWER2
#endif
#ifdef CTL_USET_GROWTH_POWER2
# undef CTL_USET_GROWTH_PRIMED
#endif

// TODO extract, merge, equal_range

#include <stdbool.h>
#if !defined(__GNUC__) && defined(CTL_USET_GROWTH_POWER2)
#include <math.h>
#endif

#define CTL_USET
#define A JOIN(uset, T)
#define B JOIN(A, node)
#define I JOIN(A, it)

#include <ctl/ctl.h>
#include <ctl/bits/iterators.h>

typedef struct B
{
    struct B* next;
    T value;
#ifdef CTL_USET_CACHED_HASH
    size_t cached_hash;
#endif
} B;

typedef struct A
{
    B** buckets;
    size_t size;
    // in STL as rehash_policy: growth_factor, max_load_factor, next_resize
    size_t bucket_count;
    size_t max_bucket_count;
    float max_load_factor;
    void (*free)(T*);
    T (*copy)(T*);
    size_t (*hash)(T*);
    int (*equal)(T*, T*);
} A;

typedef struct I
{
    B *node; // the bucket
    T* ref;
    B *next;
    //B* end; // if ranges were added to usets
    A* container;
    B** buckets; // the chain
} I;

static inline size_t
JOIN(I, index)(A* self, T value)
{
    return self->hash(&value) % self->bucket_count;
}

#ifdef CTL_USET_CACHED_HASH
static inline size_t
JOIN(I, cached_index)(A* self, B* node)
{
    return node->cached_hash % self->bucket_count;
}
#define BUCKET_INDEX(iter)                              \
    JOIN(I, cached_index)((iter)->container, (iter)->node)

#else
# define BUCKET_INDEX(iter)                             \
    JOIN(I, index)((iter)->container, (iter)->node->value)
#endif

static inline I
JOIN(I, iter)(A* self, B *node)
{
    static I zero;
    I iter = zero;
    iter.node = node;
    if (node)
    {
        iter.next = node->next;
        iter.ref = &node->value;
    }
    iter.container = self;
    iter.buckets = &self->buckets[ BUCKET_INDEX(&iter) ];
    return iter;
}

static inline I
JOIN(A, begin)(A* self)
{
    static I zero;
    I iter = zero;
    B** bend = &self->buckets[self->bucket_count];
    iter.container = self;
    for(B** b = self->buckets; b < bend; b++)
    {
        if(*b) {
            B* node = *b;
            iter.ref = &node->value;
            iter.node = node;
            iter.next = node->next;
            iter.buckets = b;
            return iter;
        }
    }
    return iter;
}

static inline I
JOIN(A, end)(A* self)
{
    static I zero;
    I iter = zero;
    iter.container = self;
    return iter;
}

static inline T*
JOIN(I, ref)(I* iter)
{
    return iter->node ? &iter->node->value : NULL;
}

// We don't support uset ranges
static inline int
JOIN(I, done)(I* iter)
{
    return iter->node == NULL; /* iter->end */
}
// unused
static inline int
JOIN(I, is_end)(I* iter, I* last)
{
    return iter->node == NULL || iter->node == last->node;
}

static inline void
JOIN(I, update)(I* iter)
{
    ASSERT(iter);
    ASSERT(iter->node);
    ASSERT(iter->buckets - iter->container->buckets < (long)iter->container->bucket_count);
    iter->ref = &iter->node->value;
    iter->next = iter->node->next;
}

/* Need two states: if next is not empty, we are still in the bucket chain.
 * if empty, we need to advance to the next bucket: buckets++.
 */
static inline void
JOIN(I, next)(I* iter)
{
    ASSERT(iter->node);
    ASSERT(iter->buckets);
    if(iter->next == NULL)
    {
        A* self = iter->container;
        B** bend = &self->buckets[self->bucket_count];
        for(B** b = ++iter->buckets; b < bend; b++)
        {
            if(*b)
            {
                iter->node = *b;
                iter->buckets = b;
                JOIN(I, update)(iter);
                return;
            }
        }
        iter->node = NULL;
    }
    else
    {
        iter->node = iter->next;
        JOIN(I, update)(iter);
    }
}

static inline I*
JOIN(I, advance)(I* iter, long i)
{
    if (i < 0)
    {
        i = iter->container->size + i;
        JOIN(A, it) it = JOIN(A, begin)(iter->container);
        iter->buckets = it.buckets;
        iter->node = it.node;
        iter->ref  = it.ref;
        iter->next = it.node->next;
    }
    for (int j = 0; j < i; j++)
        JOIN(I, next)(iter);
    return iter;
}

static inline T*
JOIN(B, ref)(B* node)
{
    return &node->value;
}

// no ranges, ignore last iters
static inline void
JOIN(I, range)(I* begin, I* end)
{
    (void) begin;
    (void) end;
}

/*
static inline void
JOIN(I, step)(I* self)
{
    if(self->next == NULL)
    {
        for(size_t i = self->bucket_index + 1; i < self->container->bucket_count; i++)
            if((self->next = self->container->buckets[i]))
            {
                JOIN(I, update)(self);
                return;
            }
        self->done = 1;
    }
    else
        JOIN(I, update)(self);
}
*/

static inline B*
JOIN(B, next)(A* container, B* node)
{
    if (node->next)
        return node->next;
    else
    {
        size_t i = JOIN(I, index)(container, node->value) + 1;
        for(; i < container->bucket_count; i++)
        {
            B* n = container->buckets[i];
            if(n)
                return n;
        }
        return NULL;
    }
}
/*
static inline I
JOIN(I, range)(A* container, B* begin, B* end)
{
    static I zero;
    I self = zero;
    if(begin)
    {
        //LOG ("range init\n");
        self.step = JOIN(I, step);
        self.node = begin;
        self.ref = &self.node->value;
        self.next = self.node->next;
        self.end = end;
        self.container = container;
        self.bucket_index = JOIN(I, index)(container, *self.ref);
    }
    else
    {
        //LOG ("range done\n");
        self.done = 1;
    }
    return self;
}
*/

static inline int
JOIN(A, _equal)(A* self, T* a, T* b)
{
    ASSERT(self->equal || !"equal undefined");
    return self->equal(a, b);
}

static inline A JOIN(A, copy)(A* self);

#include <ctl/bits/container.h>

static inline size_t
JOIN(A, __next_prime)(size_t number)
{
    static const uint32_t primes[] = {
        2, 3, 5, 7, 11,
        13, 17, 19, 23, 29, 31,
        37, 41, 43, 47, 53, 59,
        61, 67, 71, 73, 79, 83,
        89, 97, 103, 109, 113, 127,
        137, 139, 149, 157, 167, 179,
        193, 199, 211, 227, 241, 257,
        277, 293, 313, 337, 359, 383,
        409, 439, 467, 503, 541, 577,
        619, 661, 709, 761, 823, 887,
        953, 1031, 1109, 1193, 1289, 1381,
        1493, 1613, 1741, 1879, 2029, 2179,
        2357, 2549, 2753, 2971, 3209, 3469,
        3739, 4027, 4349, 4703, 5087, 5503,
        5953, 6427, 6949, 7517, 8123, 8783,
        9497, 10273, 11113, 12011, 12983, 14033,
        15173, 16411, 17749, 19183, 20753, 22447,
        24281, 26267, 28411, 30727, 33223, 35933,
        38873, 42043, 45481, 49201, 53201, 57557,
        62233, 67307, 72817, 78779, 85229, 92203,
        99733, 107897, 116731, 126271, 136607, 147793,
        159871, 172933, 187091, 202409, 218971, 236897,
        256279, 277261, 299951, 324503, 351061, 379787,
        410857, 444487, 480881, 520241, 562841, 608903,
        658753, 712697, 771049, 834181, 902483, 976369,
        1056323, 1142821, 1236397, 1337629, 1447153, 1565659,
        1693859, 1832561, 1982627, 2144977, 2320627, 2510653,
        2716249, 2938679, 3179303, 3439651, 3721303, 4026031,
        4355707, 4712381, 5098259, 5515729, 5967347, 6456007,
        6984629, 7556579, 8175383, 8844859, 9569143, 10352717,
        11200489, 12117689, 13109983, 14183539, 15345007, 16601593,
        17961079, 19431899, 21023161, 22744717, 24607243, 26622317,
        28802401, 31160981, 33712729, 36473443, 39460231, 42691603,
        46187573, 49969847, 54061849, 58488943, 63278561, 68460391,
        74066549, 80131819, 86693767, 93793069, 101473717, 109783337,
        118773397, 128499677, 139022417, 150406843, 162723577, 176048909,
        190465427, 206062531, 222936881, 241193053, 260944219, 282312799,
        305431229, 330442829, 357502601, 386778277, 418451333, 452718089,
        489790921, 529899637, 573292817, 620239453, 671030513, 725980837,
        785430967, 849749479, 919334987, 994618837, 1076067617, 1164186217,
        1259520799, 1362662261, 1474249943, 1594975441, 1725587117
    };
    size_t min = primes[0];
    if(number < min)
        return min;
    size_t size = len(primes);
    for(size_t i = 0; i < size - 1; i++)
    {
        size_t a = primes[i + 0];
        size_t b = primes[i + 1];
        if(number >= a && number <= b)
            return number == a ? a : b;
    }
    return primes[size - 1];
}

#ifdef CTL_USET_GROWTH_POWER2
// Support huge hash tables with wordsize 64? currently we have a 32bit max_size
static inline uint32_t
JOIN(A, __next_power2)(uint32_t n)
{
#ifdef __GNUC__
    return n >= 8 ? 1 << (32 - __builtin_clz(n-1)) : 8;
#else
    return 1 << (uint32_t)ceil(log2((double)n));
#endif
}
#endif

static inline B*
JOIN(B, init)(T value)
{
    B* n = (B*) malloc(sizeof(B));
    n->value = value;
    n->next = NULL;
    return n;
}

#ifdef CTL_USET_CACHED_HASH
static inline B*
JOIN(B, init_cached)(T value, size_t hash)
{
    B* n = (B*) malloc(sizeof(B));
    n->value = value;
    n->cached_hash = hash;
    n->next = NULL;
    return n;
}
#endif

static inline size_t
JOIN(B, bucket_size)(B* self)
{
    size_t size = 0;
    for(B* n = self; n; n = n->next)
        size += 1;
    return size;
}

static inline void
JOIN(B, push)(B** bucketp, B* n)
{
#ifdef DEBUG_COLL
    if (n->next)
        LOG ("push collision %zu\n", JOIN(B, bucket_size)(n));
#endif
    n->next = *bucketp;
    *bucketp = n;
}

static inline B**
JOIN(A, _cached_bucket)(A* self, B* node)
{
#ifdef CTL_USET_CACHED_HASH
    size_t hash = node->cached_hash % self->bucket_count;
#else
    size_t hash = self->hash(&node->value) % self->bucket_count;
#endif
    //LOG ("hash -> buckets[%lu]\n", hash);
    return &self->buckets[hash];
}

static inline B**
JOIN(A, _bucket_hash)(A* self, size_t hash)
{
    return &self->buckets[ hash % self->bucket_count ];
}

static inline B**
JOIN(A, _bucket)(A* self, T value)
{
    const size_t hash = self->hash(&value) % self->bucket_count;
    //LOG ("hash -> buckets[%lu]\n", hash);
    return &self->buckets[hash];
}

static inline size_t
JOIN(A, bucket)(A* self, T value)
{
    return self->hash(&value) % self->bucket_count;
}

static inline size_t
JOIN(A, bucket_size)(A* self, size_t index)
{
    size_t size = 0;
    for(B* n = self->buckets[index]; n; n = n->next)
        size++;
    return size;
}

static inline void
JOIN(A, _free_node)(A* self, B* n)
{
#ifndef POD
    ASSERT(self->free || !"no uset free without POD");
    if(self->free)
        self->free(&n->value);
#else
    ASSERT(!self->free || !"uset free with POD");
#endif
    free(n);
    self->size--;
}

static inline void
JOIN(A, max_load_factor)(A* self, float f)
{
    self->max_load_factor = f;
}

static inline size_t
JOIN(A, max_bucket_count)(A* self)
{
    return (size_t) (self->size / self->max_load_factor);
}

static inline float
JOIN(A, load_factor)(A* self)
{
    return (float) self->size / (float) self->bucket_count;
}

// new_size must fit the growth policy: power2 or prime
static inline void
JOIN(A, _reserve)(A* self, const size_t new_size)
{
    if (self->bucket_count == new_size)
        return;
    if (self->buckets)
    {
        //LOG("_reserve %zu realloc => %zu\n", self->bucket_count, new_size);
        self->buckets = (B**) realloc(self->buckets, new_size * sizeof(B*));
        if (new_size > self->bucket_count)
            memset(&self->buckets[self->bucket_count], 0,
                   (new_size - self->bucket_count) * sizeof(B*));
    }
    else
    {
        //LOG("_reserve %zu calloc => %zu\n", self->bucket_count, new_size);
        self->buckets = (B**) calloc(new_size, sizeof(B*));
    }
    self->bucket_count = new_size;
    if (self->size > 127)
        self->max_bucket_count = JOIN(A, max_bucket_count(self));
    else
        self->max_bucket_count = new_size; // ignore custom load factors here
    ASSERT (self->buckets && "out of memory");
}

static inline void JOIN(A, _rehash)(A* self, size_t count);

static inline void
JOIN(A, reserve)(A* self, size_t desired_count)
{
    if ((int32_t)desired_count <= 0)
        return;
#ifdef CTL_USET_GROWTH_POWER2
    const size_t new_size = JOIN(A, __next_power2)(desired_count);
    //LOG("power2 growth policy %zu => %zu ", desired_count, new_size);
#else
    const size_t new_size = JOIN(A, __next_prime)(desired_count);
    //LOG("primed growth policy %zu => %zu ", desired_count, new_size);
#endif
    if (new_size <= self->bucket_count)
        return;
    JOIN(A, _rehash)(self, new_size);
}

static inline A
JOIN(A, init)(size_t (*_hash)(T*), int (*_equal)(T*, T*))
{
    static A zero;
    A self = zero;
    self.hash = _hash;
    self.equal = _equal;
#ifdef POD
    self.copy = JOIN(A, implicit_copy);
    _JOIN(A, _set_default_methods)(&self);
#else
    self.free = JOIN(T, free);
    self.copy = JOIN(T, copy);
#endif
    JOIN(A, max_load_factor)(&self, 1.0f); // better would be 0.95
    JOIN(A, _reserve)(&self, 8);
    return self;
}

static inline void
JOIN(A, rehash)(A* self, size_t desired_count)
{
    if (desired_count == self->bucket_count)
        return;
    A rehashed = JOIN(A, init)(self->hash, self->equal);
    JOIN(A, reserve)(&rehashed, desired_count);
    foreach(A, self, it)
    {
        B** buckets = JOIN(A, _cached_bucket)(&rehashed, it.node);
        if (it.node != *buckets)
            JOIN(B, push)(buckets, it.node);
    }
    rehashed.size = self->size;
    //LOG ("rehash temp. from %lu to %lu, load %f\n", rehashed.size, rehashed.bucket_count,
    //     JOIN(A, load_factor)(self));
    free(self->buckets);
    //LOG ("free old\n");
    *self = rehashed;
}

// count: guaranteed growth policy (power2 or prime)
static inline void
JOIN(A, _rehash)(A* self, size_t count)
{
    // we do allow shrink here
    if (count == self->bucket_count)
        return;
    A rehashed = JOIN(A, init)(self->hash, self->equal);
    //LOG("_rehash %zu => %zu\n", self->size, count);
    JOIN(A, _reserve)(&rehashed, count);
    foreach(A, self, it)
    {
        B** buckets = JOIN(A, _cached_bucket)(&rehashed, it.node);
        if (it.node != *buckets)
            JOIN(B, push)(buckets, it.node);
    }
    rehashed.size = self->size;
    //LOG ("_rehash from %lu to %lu, load %f\n", rehashed.size, count,
    //     JOIN(A, load_factor)(self));
    free(self->buckets);
    *self = rehashed;
    return;
}

// Note: As this is used internally a lot, don't consume (free) the key.
// The user must free it by himself.
static inline B*
JOIN(A, find_node)(A* self, T value)
{
    if(self->size)
    {
#ifdef CTL_USET_CACHED_HASH
        size_t hash = self->hash(&value);
        B** buckets = JOIN(A, _bucket_hash)(self, hash);
#else
        B** buckets = JOIN(A, _bucket)(self, value);
#endif
        for(B* n = *buckets; n; n = n->next)
        {
#ifdef CTL_USET_CACHED_HASH
            // faster unsucessful searches
            if(n->cached_hash != hash)
                continue;
#endif
            if(self->equal(&value, &n->value))
                // TODO move-to-front
                return n;
        }
    }
    return NULL;
}

static inline I
JOIN(A, find)(A* self, T value)
{
    B* node = JOIN(A, find_node)(self, value);
    if (node)
        return JOIN(I, iter)(self, node);
    else
        return JOIN(A, end)(self);
}

static inline B**
JOIN(A, push_cached)(A* self, T* value)
{
#ifdef CTL_USET_CACHED_HASH
    size_t hash = self->hash(value);
    B** buckets = JOIN(A, _bucket_hash)(self, hash);
    JOIN(B, push)(buckets, JOIN(B, init_cached)(*value, hash));
#else
    B** buckets = JOIN(A, _bucket)(self, *value);
    JOIN(B, push)(buckets, JOIN(B, init)(*value));
#endif
    //LOG ("push_bucket[%zu]\n", JOIN(B, bucket_size)(*buckets));
    self->size++;
    return buckets;
}

// the various growth polices before insert
static inline void
JOIN(A, _pre_insert_grow)(A* self)
{
    if(!self->bucket_count)
        JOIN(A, rehash)(self, 8);
    if (self->size + 1 > self->max_bucket_count)
    {
#ifdef CTL_USET_GROWTH_POWER2
        const size_t bucket_count = 2 * self->bucket_count;
        //LOG ("rehash from %lu to %lu, load %f\n", self->size, self->bucket_count,
        //     JOIN(A, load_factor)(self));
        JOIN(A, _rehash)(self, bucket_count);
#else
        // The natural growth factor is the golden ratio. libstc++ v3 and
        // libc++ use 2.0 here.
        //size_t const bucket_count = (size_t)((double)self->bucket_count * 1.618033);
        size_t const bucket_count = 1.618 * (double)self->bucket_count;
        JOIN(A, rehash)(self, bucket_count);
#endif
    }
}


static inline void
JOIN(A, insert)(A* self, T value)
{
    if(JOIN(A, find_node)(self, value))
    {
        FREE_VALUE(self, value);
    }
    else
    {
        JOIN(A, _pre_insert_grow)(self);
        JOIN(A, push_cached)(self, &value);
    }
}

static inline I
JOIN(A, insert_found)(A* self, T value, int* foundp)
{
    B* node;
    if((node = JOIN(A, find_node)(self, value)))
    {
        FREE_VALUE(self, value);
        *foundp = 1;
        return JOIN(I, iter)(self, node);
    }
    *foundp = 0;
    JOIN(A, _pre_insert_grow)(self);
    node = *JOIN(A, push_cached)(self, &value);
    return JOIN(I, iter)(self, node);
}

static inline I
JOIN(A, emplace)(A* self, T* value)
{
    B* node;
    if ((node = JOIN(A, find_node)(self, *value)))
        return JOIN(I, iter)(self, node);

    JOIN(A, _pre_insert_grow)(self);
    node = *JOIN(A, push_cached)(self, value);
    return JOIN(I, iter)(self, node);
}

#ifdef DEBUG

static inline I
JOIN(A, emplace_found)(A* self, T* value, int* foundp)
{
    B* node;
    if ((node = JOIN(A, find_node)(self, *value)))
    {
        if(!self->bucket_count)
            JOIN(A, rehash)(self, 12);
        // new value, need to recalc hash
        JOIN(A, push_cached)(self, value);
        if (self->size > self->max_bucket_count)
        {
            size_t max_bucket_count = JOIN(A, max_bucket_count)(self);
            size_t new_size = JOIN(A, __next_prime)(max_bucket_count);
            JOIN(A, rehash)(self, new_size);
        }
        *foundp = 1;
        return JOIN(I, iter)(self, node);
    }

    JOIN(A, _pre_insert_grow)(self);
    *foundp = 0;
    node = *JOIN(A, push_cached)(self, value);
    return JOIN(I, iter)(self, node);
}

static inline I
JOIN(A, emplace_hint)(I* pos, T* value)
{
    A* self = pos->container;
    if (!JOIN(I, done)(pos))
    {
        // TODO: equal check if already exists
#ifdef CTL_USET_CACHED_HASH
        size_t hash = self->hash(value);
        B* node = JOIN(B, init_cached)(*value, hash);
        B** buckets = JOIN(A, _bucket_hash)(self, hash);
#else
        B* node = JOIN(B, init)(*value);
        B** buckets = JOIN(A, _bucket)(self, *value);
#endif
        for(B* n = *buckets; n; n = n->next)
        {
#ifdef CTL_USET_CACHED_HASH
            if(n->cached_hash != hash)
                continue;
#endif
            if(self->equal(value, &n->value))
                // TODO move-to-front
                return JOIN(I, iter)(self, n);
        }
        // not found
        JOIN(B, push)(buckets, node);
        pos->container->size++;
        pos->node = node;
        JOIN(I, update)(pos);
        pos->buckets = buckets;
        return *pos;
    }
    else
    {
        JOIN(A, _pre_insert_grow)(self);
        B* node = *JOIN(A, push_cached)(self, value);
        return JOIN(I, iter)(self, node);
    }
}

#endif

static inline void
JOIN(A, clear)(A* self)
{
    for(size_t i = 0; i < self->bucket_count; i++)
    {
        B* next;
        B* n = self->buckets[i];
        if (!n)
            continue;
        while ((next = n->next))
        {
            JOIN(A, _free_node)(self, n);
            n = next;
        }
        JOIN(A, _free_node)(self, n);
    }
    memset(self->buckets, 0, self->bucket_count * sizeof(B*));
    /* for(size_t i = 0; i < self->bucket_count; i++)
        self->buckets[i] = NULL; */
    self->size = 0;
    //self->max_bucket_count = 0;
}

static inline void
JOIN(A, free)(A* self)
{
    //LOG("free calloc %zu, %zu\n", self->bucket_count, self->size);
    JOIN(A, clear)(self);
    free(self->buckets);
    self->buckets = NULL;
    self->bucket_count = 0;
}

static inline size_t
JOIN(A, count)(A* self, T value)
{
    if (JOIN(A, find_node)(self, value))
    {
        FREE_VALUE(self, value);
        // TODO: the popular move-to-front strategy
        return 1UL;
    }
    else
    {
        FREE_VALUE(self, value);
        return 0UL;
    }
}

// C++20
static inline bool
JOIN(A, contains)(A* self, T value)
{
    if (JOIN(A, find_node)(self, value))
    {
        FREE_VALUE(self, value);
        // TODO: the popular move-to-front strategy
        return true;
    }
    else
    {
        FREE_VALUE(self, value);
        return false;
    }
}

static inline void
JOIN(A, _linked_erase)(A* self, B** bucket, B* n, B* prev, B* next)
{
    JOIN(A, _free_node)(self, n);
    if(prev)
        prev->next = next;
    else
        *bucket = next;
}

static inline void
JOIN(A, erase)(A* self, T value)
{
#ifdef CTL_USET_CACHED_HASH
    size_t hash = self->hash(&value);
    B** buckets = JOIN(A, _bucket_hash)(self, hash);
#else
    B** buckets = JOIN(A, _bucket)(self, value);
#endif
    B* prev = NULL;
    B* n = *buckets;
    while (n)
    {
        B* next = n->next;
#ifdef CTL_USET_CACHED_HASH
        if(n->cached_hash != hash)
        {
            prev = n;
            n = next;
            continue;
        }
#endif
        if(self->equal(&value, &n->value))
        {
            JOIN(A, _linked_erase)(self, buckets, n, prev, next);
            break;
        }
        prev = n;
        n = next;
    }
}

static inline size_t
JOIN(A, erase_if)(A* self, int (*_match)(T*))
{
    size_t erases = 0;
    for(size_t i = 0; i < self->bucket_count; i++)
    {
        B** buckets = &self->buckets[i];
        B* prev = NULL;
        B* n = *buckets;
        while(n)
        {
            B* next = n->next;
            if(_match(&n->value))
            {
                JOIN(A, _linked_erase)(self, buckets, n, prev, next);
                erases += 1;
            }
            else
                prev = n;
            n = next;
        }
    }
    return erases;
}

static inline A
JOIN(A, copy)(A* self)
{
    //LOG ("copy\norig size: %lu\n", self->size);
    A other = JOIN(A, init)(self->hash, self->equal);
    JOIN(A, _reserve)(&other, self->bucket_count);
    foreach(A, self, it) {
        //LOG ("size: %lu\n", other.size);
        JOIN(A, insert)(&other, self->copy(it.ref));
    }
    //LOG ("final size: %lu\n", other.size);
    return other;
}

static inline A
JOIN(A, union)(A* a, A* b)
{
    A self = JOIN(A, init)(a->hash, a->equal);
    JOIN(A, _reserve)(&self, MAX(a->bucket_count, b->bucket_count));
    foreach(A, a, it1)
        JOIN(A, insert)(&self, self.copy(it1.ref));
    foreach(A, b, it2)
        JOIN(A, insert)(&self, self.copy(it2.ref));
    return self;
}

static inline A
JOIN(A, intersection)(A* a, A* b)
{
    A self = JOIN(A, init)(a->hash, a->equal);
    foreach(A, a, it)
        if(JOIN(A, find_node)(b, *it.ref))
            JOIN(A, insert)(&self, self.copy(it.ref));
    return self;
}

static inline A
JOIN(A, difference)(A* a, A* b)
{
    A self = JOIN(A, init)(a->hash, a->equal);
    foreach(A, a, it)
        if(!JOIN(A, find_node)(b, *it.ref))
            JOIN(A, insert)(&self, self.copy(it.ref));
    /*
    A self = JOIN(A, copy)(a);
    foreach(A, b, it)
        JOIN(A, erase)(&self, *it.ref);
    */
    return self;
}

static inline A
JOIN(A, symmetric_difference)(A* a, A* b)
{
    A self = JOIN(A, union)(a, b);
    foreach(A, a, it)
        if(JOIN(A, find_node)(b, *it.ref))
            JOIN(A, erase)(&self, *it.ref);
    return self;
}

// different to the shared equal
static inline int
JOIN(A, equal)(A* self, A* other)
{
    size_t count_a = 0;
    size_t count_b = 0;
    foreach(A, self, it)
        if(JOIN(A, find_node)(self, *it.ref))
            count_a += 1;
    foreach(A, other, it2)
        if(JOIN(A, find_node)(other, *it2.ref))
            count_b += 1;
    return count_a == count_b;
}

static inline void
JOIN(A, swap)(A* self, A* other)
{
    A temp = *self;
    *self = *other;
    *other = temp;
}

#if defined(CTL_UMAP)
# include <ctl/algorithm.h>
#endif

#undef POD
#ifndef HOLD
#undef A
#undef B
#undef I
#undef T
#else
#undef HOLD
#endif
#undef CTL_USET

#ifdef USE_INTERNAL_VERIFY
#undef USE_INTERNAL_VERIFY
#endif
