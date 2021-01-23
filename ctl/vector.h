/* Arrays that can change in size.
   SPDX-License-Identifier: MIT */

// TODO emplace, emplace_back
// TODO end of empty vec follows NULL value
// TODO redesigned iters

#ifndef T
# error "Template type T undefined for <ctl/vector.h>"
#endif

#define CTL_VEC
#define A JOIN(vec, T)
#define I JOIN(A, it)
//#ifndef C
//# define C vec
//#endif
//#undef IT
//#define IT T*

#include <ctl/ctl.h>
#include <ctl/bits/iterators.h>

// only for short strings, not vec_uint8_t
#ifndef MUST_ALIGN_16
# define MUST_ALIGN_16(T) 0
# define INIT_SIZE 1
#else
# define INIT_SIZE 15
#endif

typedef struct A
{
    T* vector;
    void (*free)(T*);
    T (*copy)(T*);
    int (*compare)(T*, T*);
    int (*equal)(T*, T*); // optional
    size_t size;
    size_t capacity;
} A;

typedef int (*JOIN(A, compare_fn))(T*, T*);

typedef struct I
{
    T* ref;
    T* end;
    A* container;
} I;

#undef _vec_begin_it
#define _vec_begin_it JOIN(JOIN(_vec, T), begin_it)
#undef _vec_end_it
#define _vec_end_it JOIN(JOIN(_vec, T), end_it)

#ifdef __cplusplus
static I _vec_begin_it = {};
static I _vec_end_it = {};
#else
static I _vec_begin_it = {0};
static I _vec_end_it = {0};
#endif

static inline size_t
JOIN(A, capacity)(A* self)
{
    return self->capacity;
}

static inline T*
JOIN(A, at)(A* self, size_t index)
{
#if defined(_ASSERT_H) && !defined(NDEBUG)
    assert (index < self->size || !"out of range");
#endif
    return index < self->size ? &self->vector[index] : NULL;
}

static inline T*
JOIN(A, front)(A* self)
{
    return &self->vector[0]; // not bounds-checked
}

static inline T*
JOIN(A, back)(A* self)
{
    return self->size ? JOIN(A, at)(self, self->size - 1) : NULL;
}

static inline I
JOIN(A, begin)(A* self)
{
    I iter = _vec_begin_it;
    iter.ref = &self->vector[0];
    iter.end = &self->vector[self->size];
    iter.container = self;
    return iter;
}

static inline I
JOIN(A, end)(A* self)
{
    I iter = _vec_end_it;
    iter.end = iter.ref = &self->vector[self->size];
    iter.container = self;
    return iter;
}

static inline I
JOIN(I, iter)(A* self, size_t index)
{
    I iter = _vec_begin_it;
    iter.ref = JOIN(A, at)(self, index); // bounds-checked
    iter.end = &self->vector[self->size];
    iter.container = self;
    return iter;
}

static inline T*
JOIN(I, ref)(I* iter)
{
    return iter->ref;
}

static inline size_t
JOIN(I, index)(I* iter)
{
    return (iter->ref - JOIN(A, front)(iter->container)) / sizeof (T);
}

static inline int
JOIN(I, isend)(I* iter, I* last)
{
    return iter->ref == last->ref;
}

static inline int
JOIN(I, done)(I* iter)
{
    return iter->ref == iter->end;
}

static inline void
JOIN(I, next)(I* iter)
{
    iter->ref++;
}

static inline void
JOIN(I, range)(I* begin, I* end)
{
    begin->end = end->ref;
}

static inline I*
JOIN(I, advance)(I* iter, long i)
{
    // error case: overflow => end or NULL?
    if(iter->ref + i > iter->end ||
       iter->ref + i < JOIN(A, front)(iter->container))
        iter->ref = iter->end;
    else
        iter->ref += i;
    return iter;
}

static inline long
JOIN(I, distance)(I* iter, I* other)
{
    return other->ref - iter->ref;
}

#include <ctl/bits/container.h>

static inline A
JOIN(A, init)(void)
{
    static A zero;
    A self = zero;
#ifdef POD
    self.copy = JOIN(A, implicit_copy);
    _JOIN(A, _set_default_methods)(&self);
#else
    self.free = JOIN(T, free);
    self.copy = JOIN(T, copy);
#endif
    return self;
}

static inline A
JOIN(A, init_from)(A* copy)
{
    static A zero;
    A self = zero;
    self.free = copy->free;
    self.copy = copy->copy;
    self.compare = copy->compare;
    self.equal = copy->equal;
    return self;
}

// not bounds-checked. like operator[]
static inline void
JOIN(A, set)(A* self, size_t index, T value)
{
    T* ref = &self->vector[index];
#ifndef POD
    if(self->free)
        self->free(ref);
#endif
    *ref = value;
}

static inline void
JOIN(A, pop_back)(A* self)
{
    static T zero;
    self->size--;
    JOIN(A, set)(self, self->size, zero);
}

static inline void
JOIN(A, fit)(A* self, size_t capacity)
{
    size_t overall = capacity;
#if defined(_ASSERT_H) && !defined(NDEBUG)
    assert (capacity < JOIN(A, max_size)() || !"max_size overflow");
#endif
    if(MUST_ALIGN_16(T)) // reserve terminating \0 for strings
        overall++;
    if (self->vector)
    {
        self->vector = (T*) realloc(self->vector, overall * sizeof(T));
        if(MUST_ALIGN_16(T))
        {
#if 0
            static T zero;
            for(size_t i = self->capacity; i < overall; i++)
                self->vector[i] = zero;
#else
            if (overall > self->capacity)
                memset (&self->vector[self->capacity], 0, (overall - self->capacity) * sizeof(T));
#endif
        }
    }
    else
        self->vector = (T*) calloc(overall, sizeof(T));
    self->capacity = capacity;
}

static inline void
JOIN(A, wipe)(A* self, size_t n)
{
    while(n != 0)
    {
        JOIN(A, pop_back)(self);
        n--;
    }
#if defined CTL_STR && defined _LIBCPP_STD_VER
    if (self->capacity <= 30)
        JOIN(A, fit)(self, 47);
#endif
}

static inline void
JOIN(A, clear)(A* self)
{
    if(self->size > 0)
        JOIN(A, wipe)(self, self->size);
}

static inline void
JOIN(A, free)(A* self)
{
    JOIN(A, clear)(self);
    JOIN(A, compare_fn) *compare = &self->compare;
    JOIN(A, compare_fn) *equal = &self->equal;
    free(self->vector);
    *self = JOIN(A, init)();
    self->compare = *compare;
    self->equal = *equal;
}

static inline void
JOIN(A, reserve)(A* self, const size_t n)
{
    const size_t max_size = JOIN(A, max_size)();
    if(n > max_size)
    {
#if defined(_ASSERT_H) && !defined(NDEBUG)
        assert (n < max_size || !"max_size overflow");
#endif
        return;
    }
#ifdef CTL_STR
    if(self->capacity != n)
#else
    // never shrink vectors with reserve
    if(self->capacity < n)
#endif
    {
        // don't shrink, but shrink_to_fit
        size_t actual = n < self->size ? self->size : n;
        if(actual > 0)
        {
#ifdef CTL_STR
            // reflecting gcc libstdc++ with __cplusplus >= 201103L < 2021 (gcc-10)
            if (actual > self->capacity) // double it
            {
                if (actual < 2 * self->capacity)
                    actual = 2 * self->capacity;
                if (actual > max_size)
                    actual = max_size;
# ifdef _LIBCPP_STD_VER
                // with libc++ round up to 16
                // which versions? this is 18 (being 2018 for clang 10)
                // but I researched it back to the latest change in __grow_by in
                // PR17148, 2013
                // TODO: Is there a _LIBCPP_STD_VER 13?
                if (actual > 30)
                    actual = ((actual & ~15) == actual)
                        ? (actual + 15)
                        : ((actual + 15) & ~15)- 1;
# endif
                JOIN(A, fit)(self, actual);
            }
            else
#endif
                JOIN(A, fit)(self, actual);
        }
    }
}

static inline void
JOIN(A, push_back)(A* self, T value)
{
    if(self->size == self->capacity)
        JOIN(A, reserve)(self,
            self->capacity == 0 ? INIT_SIZE : 2 * self->capacity);
    self->size++;
    *JOIN(A, at)(self, self->size - 1) = value;
//#ifdef CTL_STR
//    self->vector[self->size] = '\0';
//#endif
}

static inline void
JOIN(A, emplace)(A* self, T* value)
{
    if(self->size == self->capacity)
        JOIN(A, reserve)(self,
            self->capacity == 0 ? INIT_SIZE : 2 * self->capacity);
    self->size++;
    *JOIN(A, at)(self, self->size - 1) = *value;
}

static inline void
JOIN(A, resize)(A* self, size_t size, T value)
{
    if(size < self->size)
    {
        int64_t less = self->size - size;
        if(less > 0)
            JOIN(A, wipe)(self, less);
    }
    else
    {
        if(size > self->capacity)
        {
#ifdef CTL_STR
            size_t capacity = 2 * self->capacity;
            if(size > capacity)
                capacity = size;
            JOIN(A, reserve)(self, capacity);
#else       // different vector growth policy. double or just grow as needed.
            size_t capacity;
            size_t n = size > self->size ? size - self->size : 0;
            LOG("  grow vector by %zu with size %zu\n", n, self->size);
            capacity = self->size + (self->size > n ? self->size : n);
            JOIN(A, fit)(self, capacity);
#endif
        }
        for(size_t i = 0; self->size < size; i++)
            JOIN(A, push_back)(self, self->copy(&value));
    }
#ifndef POD
    if(self->free)
        self->free(&value);
#endif
}

static inline void
JOIN(A, assign)(A* self, size_t count, T value)
{
    JOIN(A, resize)(self, count, self->copy(&value));
    for(size_t i = 0; i < count; i++)
        JOIN(A, set)(self, i, self->copy(&value));
#ifndef POD
    if(self->free)
        self->free(&value);
#endif
}

static inline void
JOIN(A, assign_range)(A* self, T* from, T* last)
{
    size_t count = last - from;
    JOIN(A, resize)(self, count, self->copy(self->vector)); // TODO
    for(size_t i = 0; i < self->size; i++)
        JOIN(A, set)(self, i, *JOIN(A, at)(self, i));
}

static inline void
JOIN(A, shrink_to_fit)(A* self)
{
    LOG("shrink_to_fit size %zu\n", self->size);
    if(MUST_ALIGN_16(T) // only for string
#ifndef _LIBCPP_STD_VER // gcc optimizes <16, llvm < 22. msvc??
        && self->size <= 15
#endif
        )
    {
        size_t size;
#ifdef _LIBCPP_STD_VER
        if (self->size < 22)
            size = 22;
        else {
            size = ((self->size + 15) & ~15) - 1;
            if (size < self->size)
                size += 16;
        }
#else
        size = self->size ? ((self->size + 15) & ~15) - 1 : 15;
#endif
        JOIN(A, fit)(self, size);
    }
    else
        JOIN(A, fit)(self, self->size);
}

static inline T*
JOIN(A, data)(A* self)
{
    return JOIN(A, front)(self);
}

static inline void
JOIN(A, insert)(A* self, size_t index, T value)
{
    if(self->size > 0)
    {
        JOIN(A, push_back)(self, *JOIN(A, back)(self));
        for(size_t i = self->size - 2; i > index; i--)
            self->vector[i] = self->vector[i - 1];
        self->vector[index] = value;
    }
    else
        JOIN(A, push_back)(self, value);
}

static inline T*
JOIN(A, erase_index)(A* self, size_t index)
{
    static T zero;
#if 1
    if(self->free)
        self->free(&self->vector[index]);
    if (index < self->size - 1)
        memmove(&self->vector[index], &self->vector[index] + 1, (self->size - index - 1) * sizeof (T));
    JOIN(A, set)(self, self->size - 1, zero);
#else
    JOIN(A, set)(self, index, zero);
    for(size_t i = index; i < self->size - 1; i++)
    {
        self->vector[i] = self->vector[i + 1];
        self->vector[i + 1] = zero;
    }
#endif
    self->size--;
    return &self->vector[index];
}

static inline T*
JOIN(A, erase_range)(A* self, T* from, T* to)
{
    static T zero;
    if (from >= to)
        return to;
    T* end = &self->vector[self->size];
#if 1
    size_t size = (to - from) / sizeof(T);
# ifndef POD
    if(self->free)
        for(T* ref = from; ref < to - 1; ref++)
            self->free(ref);
# endif
    if (to != end)
    {
        memmove(from, to, (end - to) * sizeof (T));
        JOIN(A, set)(self, self->size - size, zero);
    }
    self->size -= size;
#else
    *from = zero;
    for(T* pos = from; pos < to - 1; pos++)
    {
        *pos = *(pos + 1);
        *(pos + 1) = zero;
        self->size--;
    }
#endif
    if (to < end)
        return to + 1;
    else
        return to;
}

static inline T*
JOIN(A, erase)(A* self, T* pos)
{
    return JOIN(A, erase_index)(self, pos - &self->vector[0]);
}

static inline void
JOIN(A, swap)(A* self, A* other)
{
    A temp = *self;
    *self = *other;
    *other = temp;
    JOIN(A, shrink_to_fit)(self);
}

static inline void
JOIN(A, _ranged_sort)(A* self, size_t a, size_t b, int _compare(T*, T*))
{
    if(UNLIKELY(a >= b))
        return;
    // TODO insertion_sort cutoff
    //long mid = (a + b) / 2; // overflow!
    // Dietz formula http://aggregate.org/MAGIC/#Average%20of%20Integers
    size_t mid = ((a ^ b) >> 1) + (a & b);
    //LOG("sort \"%s\" %ld, %ld\n", self->vector, a, b);
    SWAP(T, &self->vector[a], &self->vector[mid]);
    size_t z = a;
    // check overflow of a + 1
    if (LIKELY(a + 1 > a))
        for(size_t i = a + 1; i <= b; i++)
            if(_compare(&self->vector[a], &self->vector[i]))
            {
                z++;
                SWAP(T, &self->vector[z], &self->vector[i]);
            }
    SWAP(T, &self->vector[a], &self->vector[z]);
    if (LIKELY(z))
        JOIN(A, _ranged_sort)(self, a, z - 1, _compare);
    // check overflow of z + 1
    if (LIKELY(z + 1 > z))
        JOIN(A, _ranged_sort)(self, z + 1, b, _compare);
}

static inline void
JOIN(A, sort)(A* self)
{
    CTL_ASSERT_COMPARE
    // TODO insertion_sort cutoff
    if (self->size > 1)
        JOIN(A, _ranged_sort)(self, 0, self->size - 1, self->compare);
//#ifdef CTL_STR
//    self->vector[self->size] = '\0';
//#endif
}

static inline A
JOIN(A, copy)(A* self)
{
    A other = JOIN(A, init_from)(self);
    JOIN(A, reserve)(&other, self->size); // i.e shrink to fit
    while(other.size < self->size)
        JOIN(A, push_back)(&other, other.copy(&self->vector[other.size]));
    return other;
}

static inline size_t
JOIN(A, remove_if)(A* self, int (*_match)(T*))
{
    size_t erases = 0;
    if (!self->size)
        return 0;
    for(size_t i = 0; i < self->size; )
    {
        if(_match(&self->vector[i]))
        {
            JOIN(A, erase_index)(self, i);
            erases++;
        }
        else
            i++;
    }
    return erases;
}

static inline size_t
JOIN(A, erase_if)(A* self, int (*_match)(T*))
{
    return JOIN(A, remove_if)(self, _match);
}

#ifndef CTL_STR
static inline I
JOIN(A, find)(A* self, T key)
{
    vec_foreach(T, self, ref)
        if (JOIN(A, _equal)(self, ref, &key))
            return JOIN(I, iter)(self, ref - &self->vector[0]);
    return JOIN(A, end(self));
}
#endif

#if defined(CTL_STR) || \
    defined(CTL_U8STR)
# include <ctl/algorithm.h>
#endif

#undef A
#undef I
#undef MUST_ALIGN_16
#undef INIT_SIZE

// Hold preserves `T` if other containers
// (eg. `priority_queue.h`) wish to extend `vector.h`.
#ifdef HOLD
#undef HOLD
#else
#undef C
#undef T
#undef POD
#undef NOT_INTEGRAL
#endif
#undef CTL_VEC
