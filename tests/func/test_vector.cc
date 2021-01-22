#include "../test.h"
#include "digi.hh"

#define T digi
#include <ctl/vector.h>

#include <vector>
#include <algorithm>

void print_vec(vec_digi *a)
{
    vec_foreach(digi, a, ref)
        printf ("%d ", *ref->value);
    printf ("\n");
}

void print_vector(std::vector<DIGI> &b)
{
    for(auto& d: b)
        printf ("%d ", *d.value);
    printf ("\n");
}

#ifdef DEBUG
#define TEST_MAX_SIZE 15
//#define TEST_MAX_VALUE INT_MAX
#define TEST_MAX_VALUE 100
#else
#define print_vec(x)
#define print_vector(x)
#define TEST_MAX_VALUE INT_MAX
#endif

int random_element(vec_digi* a)
{
    const size_t index = TEST_RAND(a->size);
    digi *vp = vec_digi_at(a, index);
    return *vp->value;
}

// tested variants
#if (defined _GLIBCXX_RELEASE && __cplusplus >= 201103L)
// Tested ok with g++ 10, g++ 7.5,
// clang 10 (libc++ 11-18), apple clang 12 fail
# define ASSERT_EQUAL_CAP(c, s) assert(s.capacity() == c.capacity);
#else
// other llvm libc++ fail (gh actions), msvc untested
# define ASSERT_EQUAL_CAP(c, s) if (s.capacity() != c.capacity) \
    { printf("capacity %zu vs %zu FAIL\n", c.capacity, s.capacity()); fail++; }
#endif

#define CHECK(_x, _y) {                                           \
    ASSERT_EQUAL_CAP(_x, _y)                                      \
    assert(_x.size == _y.size());                                 \
    assert(vec_digi_empty(&_x) == _y.empty());                    \
    if(_x.size > 0) {                                             \
        assert(*_y.front().value == *vec_digi_front(&_x)->value); \
        assert(*_y.back().value == *vec_digi_back(&_x)->value);   \
    }                                                             \
    std::vector<DIGI>::iterator _iter = _y.begin();               \
    vec_foreach(digi, &_x, _ref) {                                \
        assert(*(_ref->value) == *_iter->value);                  \
        _iter++;                                                  \
    }                                                             \
    digi* _ref = vec_digi_front(&_x);                             \
    for(auto& _d : _y) {                                          \
        assert(*(_ref->value) == *_d.value);                      \
        _ref++;                                                   \
    }                                                             \
    for(size_t i = 0; i < _y.size(); i++)                         \
        assert(*_y.at(i).value == *vec_digi_at(&_x, i)->value);   \
}

#define CHECK_ITER(_it, b, _iter)                                 \
    if (!vec_digi_it_done(_it))                                   \
    {                                                             \
        assert (_iter != b.end());                                \
        assert(*((_it)->ref->value) == *(*_iter).value);          \
    } else                                                        \
        assert (_iter == b.end())

#define CHECK_REF(_ref, b, _iter)                                 \
    if (_iter != b.end())                                         \
        assert(*(_ref->value) == *(*_iter).value)

#ifdef DEBUG
static void
get_random_iters (vec_digi *a, vec_digi_it* first_a, vec_digi_it* last_a,
                  std::vector<DIGI>& b, std::vector<DIGI>::iterator &first_b,
                  std::vector<DIGI>::iterator &last_b)
{
    size_t r1 = TEST_RAND(a->size / 2);
    const size_t rnd = TEST_RAND(a->size / 2);
    size_t r2 = MIN(r1 + rnd, a->size);
    LOG("iters %zu, %zu of %zu\n", r1, r2, a->size);
    if (a->size)
    {
        vec_digi_it it1 = vec_digi_begin(a);
        first_b = b.begin();
        vec_digi_it_advance(&it1, r1);
        first_b += r1;
        *first_a = it1;

        if (r1 == r2)
        {
            *last_a = it1;
            last_b = first_b;
        }
        else if (r2 == a->size)
        {
            *last_a = vec_digi_end(a);
            last_b = b.end();
        }
        else
        {
            vec_digi_it it2 = vec_digi_begin(a);
            last_b = b.begin();
            vec_digi_it_advance(&it2, r2);
            last_b += r2;
            *last_a = it2;
        }
    }
    else
    {
        vec_digi_it end = vec_digi_end(a);
        *first_a = end;
        *last_a = end;
        first_b = b.begin();
        last_b = b.end();
    }
}
#endif

int
main(void)
{
    int fail = 0;
    INIT_SRAND;
    INIT_TEST_LOOPS(10);
    for(size_t loop = 0; loop < loops; loop++)
    {
        size_t size = TEST_RAND(TEST_MAX_SIZE);
        enum
        {
            MODE_DIRECT,
            MODE_GROWTH,
            MODE_TOTAL
        };
        for(size_t mode = MODE_DIRECT; mode < MODE_TOTAL; mode++)
        {
            vec_digi a = vec_digi_init();
            a.compare = digi_compare;
            a.equal = digi_equal;
            std::vector<DIGI> b;
            if(mode == MODE_DIRECT)
            {
                LOG("mode DIRECT\n");
                vec_digi_resize(&a, size, digi_init(0));
                b.resize(size);
            }
            if(mode == MODE_GROWTH)
            {
                LOG("mode GROWTH\n");
                for(size_t pushes = 0; pushes < size; pushes++)
                {
                    const int value = TEST_RAND(INT_MAX);
                    vec_digi_push_back(&a, digi_init(value));
                    b.push_back(DIGI{value});
                }
            }

#define FOREACH_METH(TEST) \
        TEST(PUSH_BACK) \
        TEST(POP_BACK) \
        TEST(CLEAR) \
        TEST(ERASE_INDEX) \
        TEST(ERASE) \
        TEST(ERASE_RANGE) \
        TEST(RESIZE) \
        TEST(RESERVE) \
        TEST(SHRINK_TO_FIT) \
        TEST(SORT) \
        TEST(COPY) \
        TEST(SWAP) \
        TEST(INSERT) \
        TEST(ASSIGN) \
        TEST(REMOVE_IF) \
        TEST(ERASE_IF) \
        TEST(EQUAL) \
        TEST(FIND) \
        TEST(ALL_OF) \
        TEST(FIND_IF) \
        TEST(FIND_IF_NOT) \
        TEST(NONE_OF) \
        TEST(COUNT) \
        TEST(COUNT_IF) \

#define FOREACH_DEBUG(TEST) \
        TEST(INSERT_COUNT) \
        TEST(INSERT_RANGE) \
        TEST(EQUAL_RANGE) \
        TEST(FIND_RANGE) \
        TEST(FIND_IF_RANGE) \
        TEST(FIND_IF_NOT_RANGE) \
        TEST(ANY_OF) \
        TEST(ALL_OF_RANGE) \
        TEST(ANY_OF_RANGE) \
        TEST(NONE_OF_RANGE) \
        TEST(COUNT_IF_RANGE) \
        TEST(COUNT_RANGE) \
        TEST(INTERSECTION) \
        TEST(DIFFERENCE) \

      /*TEST(FIND_END)
        TEST(FIND_END_RANGE)
      */

#define GENERATE_ENUM(x) TEST_##x,
#define GENERATE_NAME(x) #x,

            enum {
                FOREACH_METH(GENERATE_ENUM)
#ifdef DEBUG
                FOREACH_DEBUG(GENERATE_ENUM)
#endif
                TEST_TOTAL
            };
#ifdef DEBUG
            static const char *test_names[] = {
                FOREACH_METH(GENERATE_NAME)
                FOREACH_DEBUG(GENERATE_NAME)
                ""
            };
#endif
            int which = TEST_RAND(TEST_TOTAL);
            if (test >= 0 && test < (int)TEST_TOTAL)
                which = test;
            if (which > TEST_COUNT_IF)
                which = 0;
            LOG ("TEST=%d %s (size %zu, cap %zu)\n", which, test_names[which], a.size, a.capacity);
            switch(which)
            {
                case TEST_PUSH_BACK:
                {
                    const int value = TEST_RAND(INT_MAX);
                    b.push_back(DIGI{value});
                    vec_digi_push_back(&a, digi_init(value));
                    break;
                }
                case TEST_POP_BACK:
                {
                    if(a.size > 0)
                    {
                        b.pop_back();
                        vec_digi_pop_back(&a);
                    }
                    break;
                }
                case TEST_CLEAR:
                {
                    b.clear();
                    vec_digi_clear(&a);
                    break;
                }
                case TEST_ERASE_INDEX:
                {
                    if(a.size > 0)
                    {
                        const size_t index = TEST_RAND(a.size);
                        auto it = b.erase(b.begin() + index);
                        digi* pos = vec_digi_erase_index(&a, index);
                        CHECK_REF(pos, b, it);
                    }
                    CHECK(a, b);
                    break;
                }
                case TEST_ERASE:
                {
                    if(a.size > 0)
                    {
                        const size_t index = TEST_RAND(a.size);
                        vec_digi_it pos = vec_digi_begin(&a);
                        vec_digi_it_advance(&pos, index);
                        digi* i = vec_digi_erase(&a, pos.ref);
                        auto it = b.erase(b.begin() + index);
                        CHECK_REF(i, b, it);
                    }
                    CHECK(a, b);
                    break;
                }
                case TEST_ERASE_RANGE:
                {
                    if(a.size > 0)
                    {
                        const size_t i1 = TEST_RAND(a.size);
                        const size_t i2 = i1 + TEST_RAND(a.size - i1);
                        vec_digi_it from = vec_digi_begin(&a);
                        vec_digi_it_advance(&from, i1);
                        vec_digi_it to = vec_digi_begin(&a);
                        vec_digi_it_advance(&from, i2);
                        digi* pos = vec_digi_erase_range(&a, from.ref, to.ref);
                        auto it = b.erase(b.begin() + i1, b.begin() + i2);
                        CHECK_REF(pos, b, it); //FIXME at end
                    }
                    CHECK(a, b);
                    break;
                }
                case TEST_INSERT:
                {
                    size_t amount = TEST_RAND(512);
                    for(size_t count = 0; count < amount; count++)
                    {
                        const int value = TEST_RAND(INT_MAX);
                        const size_t index = TEST_RAND(a.size);
                        b.insert(b.begin() + index, DIGI{value});
                        vec_digi_insert(&a, index, digi_init(value));
                    }
                    break;
                }
                case TEST_RESIZE:
                {
                    const size_t resize = 3 * TEST_RAND(a.size) + 1;
                    b.resize(resize);
                    LOG("STL resize by %zu %zu -> %zu\n", resize, b.size(), b.capacity());
                    vec_digi_resize(&a, resize, digi_init(0));
                    LOG("CTL resize by %zu %zu -> %zu\n", resize, a.size, a.capacity);
                    break;
                }
                case TEST_RESERVE:
                {
                    const size_t capacity = 3 * TEST_RAND(a.capacity) + 1;
                    b.reserve(capacity);
                    vec_digi_reserve(&a, capacity);
                    LOG("CTL reserve by %zu %zu\n", capacity, a.capacity);
                    LOG("STL reserve by %zu %zu\n", capacity, b.capacity());
                    break;
                }
                case TEST_SHRINK_TO_FIT:
                {
                    b.shrink_to_fit();
                    vec_digi_shrink_to_fit(&a);
                    LOG("CTL shrink_to_fit %zu %zu\n", a.size, a.capacity);
                    LOG("STL shrink_to_fit %zu %zu\n", b.size(), b.capacity());
                    break;
                }
                case TEST_SORT:
                {
                    vec_digi_sort(&a);
                    std::sort(b.begin(), b.end());
                    break;
                }
                case TEST_COPY:
                {
                    vec_digi aa = vec_digi_copy(&a);
                    std::vector<DIGI> bb = b;
                    CHECK(aa, bb);
                    vec_digi_free(&aa);
                    break;
                }
                case TEST_ASSIGN:
                {
                    const int value = TEST_RAND(INT_MAX);
                    size_t assign_size = TEST_RAND(a.size) + 1;
                    vec_digi_assign(&a, assign_size, digi_init(value));
                    b.assign(assign_size, DIGI{value});
                    break;
                }
                case TEST_SWAP:
                {
                    LOG("CTL capacity %zu\n", a.capacity);
                    LOG("STL capacity %zu\n", b.capacity());
                    vec_digi aa = vec_digi_copy(&a);
                    vec_digi aaa = vec_digi_init();
                    LOG("CTL capacity %zu copy %zu\n", aa.capacity, aa.size);
                    LOG("CTL capacity %zu init\n", aaa.capacity);
                    std::vector<DIGI> bb = b;
                    std::vector<DIGI> bbb;
                    vec_digi_swap(&aaa, &aa);
                    LOG("CTL capacity %zu after swap %zu\n", aaa.capacity, aaa.size);
                    std::swap(bb, bbb);
                    LOG("STL capacity %zu after swap %zu\n", bbb.capacity(), bbb.size());
                    CHECK(aaa, bbb);
                    vec_digi_free(&aaa);
                    break;
                }
                case TEST_REMOVE_IF:
                {
                    vec_digi_remove_if(&a, digi_is_odd);
                    b.erase(std::remove_if(b.begin(), b.end(), DIGI_is_odd), b.end());
                    break;
                }
                case TEST_ERASE_IF:
                {
#if __cpp_lib_erase_if > 202002L
                    size_t num_a = vec_digi_erase_if(&a, digi_is_odd);
                    size_t num_b = b.erase_if(DIGI_is_odd);
                    assert(num_a == num_b);
#else
                    vec_digi_erase_if(&a, digi_is_odd);
                    b.erase(std::remove_if(b.begin(), b.end(), DIGI_is_odd), b.end());
#endif
                    break;
                }
                case TEST_EQUAL:
                {
                    vec_digi aa = vec_digi_copy(&a);
                    std::vector<DIGI> bb = b;
                    assert(vec_digi_equal(&a, &aa));
                    assert(b == bb);
                    vec_digi_free(&aa);
                    break;
                }
                case TEST_FIND:
                {
                    if(a.size > 0)
                    {
                        const size_t index = TEST_RAND(a.size);
                        int value = TEST_RAND(2) ? TEST_RAND(INT_MAX) : *vec_digi_at(&a, index)->value;
                        digi key = digi_init(value);
                        digi* aa = vec_digi_find(&a, key);
                        auto bb = find(b.begin(), b.end(), DIGI{value});
                        bool found_a = aa != NULL;
                        bool found_b = bb != b.end();
                        assert(found_a == found_b);
                        if(found_a && found_b)
                            assert(*(aa.ref->value) == *bb->value);
                        digi_free(&key);
                    }
                    break;
                }
#ifdef DEBUG
                case TEST_EQUAL_RANGE:
                    // TODO
                    break;
#if 0
                case TEST_FIND_END:
                {
                    if(a.size > 0)
                    {
                        vec_digi_it first_a, last_a;
                        vec_digi_it aa = vec_digi_find_end(&a, &s_first, &s_last);
                        auto bb = find_end(b.begin(), b.end(), ...);
                        bool found_a = !vec_digi_it_done(&aa);
                        bool found_b = bb != b.end();
                        assert(found_a == found_b);
                        if(found_a && found_b)
                            assert(*(aa->value) == *bb->value);
                    }
                    break;
                }
#endif
                case TEST_FIND_RANGE:
                {
                    int vb = TEST_RAND(2) ? TEST_RAND(TEST_MAX_VALUE)
                        : random_element(&a);
                    digi key = digi_init(vb);
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    first_a = vec_digi_find_range(&first_a, &last_a, key);
                    auto it = find(first_b, last_b, vb);
                    CHECK_ITER(&first_a, b, it);
                    digi_free (&key); // special
                    CHECK(a, b);
                    break;
                }
#if 0
                case TEST_FIND_END_RANGE:
                {
                    vec_digi_it first_a, last_a, s_first, s_last;
                    std::vector<DIGI>::iterator first_b, last_b, s_first_b, s_last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
# if __cpp_lib_erase_if > 202002L
                    first_a = vec_digi_find_end_range(&first_a, &last_a, &s_first_a, &s_last_a);
                    auto it = find_end(first_b, last_b, vb);
                    CHECK_ITER(&first_a, b, it);
                    CHECK(a, b);
# endif
                    break;
                }
                case TEST_FIND_END_IF_RANGE:
                {
                    vec_digi_it first_a, last_a, s_first, s_last;
                    std::vector<DIGI>::iterator first_b, last_b, s_first_b, s_last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
# if __cpp_lib_erase_if > 202002L
                    first_a = vec_digi_find_end_if_range(&first_a, &last_a, &s_first, &s_last, digi_is_odd);
                    auto it = find_end(first_b, last_b, s_first_b, s_last_b, DIGI_is_odd);
                    CHECK_ITER(&first_a, b, it);
                    digi_free (&key); // special
                    CHECK(a, b);
# endif
                    break;
                }
#endif
                case TEST_FIND_IF_RANGE:
                {
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    first_a = vec_digi_find_if_range(&first_a, &last_a, digi_is_odd);
                    auto it = find_if(first_b, last_b, DIGI_is_odd);
                    print_vec(&a);
                    print_vector(b);
                    CHECK_ITER(&first_a, b, it);
                    break;
                }
                case TEST_FIND_IF_NOT_RANGE:
                {
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    first_a = vec_digi_find_if_not_range(&first_a, &last_a, digi_is_odd);
                    auto it = find_if_not(first_b, last_b, DIGI_is_odd);
                    CHECK_ITER(&first_a, b, it);
                    break;
                }
                case TEST_ALL_OF_RANGE:
                {
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    bool aa = vec_digi_all_of_range(&first_a, &last_a, digi_is_odd);
                    bool bb = all_of(first_b, last_b, DIGI_is_odd);
                    if (aa != bb)
                    {
                        print_vec(&a);
                        print_vector(b);
                        printf ("%d != %d is_odd\n", (int)aa, (int)bb);
                    }
                    assert(aa == bb);
                    break;
                }
                case TEST_ANY_OF_RANGE:
                {
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    bool aa = vec_digi_any_of_range(&first_a, &last_a, digi_is_odd);
                    bool bb = any_of(first_b, last_b, DIGI_is_odd);
                    if (aa != bb)
                    {
                        print_vec(&a);
                        print_vector(b);
                        printf ("%d != %d is_odd\n", (int)aa, (int)bb);
                    }
                    assert(aa == bb);
                    break;
                }
                case TEST_NONE_OF_RANGE:
                {
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    bool aa = vec_digi_none_of_range(&first_a, &last_a, digi_is_odd);
                    bool bb = none_of(first_b, last_b, DIGI_is_odd);
                    if (aa != bb)
                    {
                        print_vec(&a);
                        print_vector(b);
                        printf ("%d != %d is_odd\n", (int)aa, (int)bb);
                    }
                    assert(aa == bb);
                    break;
                }
                case TEST_COUNT_IF_RANGE:
                {
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    size_t numa = vec_digi_count_if_range(&first_a, &last_a, digi_is_odd);
                    size_t numb = count_if(first_b, last_b, DIGI_is_odd);
                    if (numa != numb)
                    {
                        print_vec(&a);
                        print_vector(b);
                        printf ("%d != %d FAIL\n", (int)numa, (int)numb);
                        fail++;
                    }
                    assert(numa == numb); //fails. off by one, counts one too much
                    break;
                }
                case TEST_COUNT_RANGE:
                {
                    int test_value = 0;
                    int v = TEST_RAND(2) ? TEST_RAND(TEST_MAX_VALUE)
                        : test_value;
                    vec_digi_it first_a, last_a;
                    std::vector<DIGI>::iterator first_b, last_b;
                    get_random_iters (&a, &first_a, &last_a, b, first_b, last_b);
                    // used to fail with 0,0 of 0
                    size_t numa = vec_digi_count_range(&first_a, &last_a, digi_init(v));
                    size_t numb = count(first_b, last_b, DIGI{v});
                    assert(numa == numb);
                    break;
                }
                case TEST_ANY_OF: // broken
                {
                    bool is_a = vec_digi_all_of(&a, digi_is_odd);
                    bool is_b = std::any_of(b.begin(), b.end(), DIGI_is_odd);
                    if (is_a != is_b)
                    {
                        //print_vec(&a);
                        //print_vector(b);
                        printf ("%d != %d FAIL\n", (int)is_a, (int)is_b);
                        fail++;
                    }
                    //assert(is_a == is_b);
                    break;
                }
#endif
                case TEST_FIND_IF:
                {
                    vec_digi_it it = vec_digi_find_if(&a, digi_is_odd);
                    auto bb = std::find_if(b.begin(), b.end(), DIGI_is_odd);
                    if(bb == b.end())
                        assert(vec_digi_it_done(&it));
                    else
                        assert(*(it.ref->value) == *bb->value);
                    break;
                }
                case TEST_FIND_IF_NOT:
                {
                    vec_digi_it aa = vec_digi_find_if_not(&a, digi_is_odd);
                    auto bb = std::find_if_not(b.begin(), b.end(), DIGI_is_odd);
                    print_vec(&a);
                    print_vector(b);
                    CHECK_ITER(&aa, b, bb);
                    if(bb == b.end())
                        assert(vec_digi_it_done(&aa));
                    else
                        assert(*(aa.ref->value) == *bb->value);
                    break;
                }
                case TEST_ALL_OF:
                {
                    bool is_a = vec_digi_all_of(&a, digi_is_odd);
                    bool is_b = std::all_of(b.begin(), b.end(), DIGI_is_odd);
                    assert(is_a == is_b);
                    break;
                }
                case TEST_NONE_OF:
                {
                    bool is_a = vec_digi_none_of(&a, digi_is_odd);
                    bool is_b = std::none_of(b.begin(), b.end(), DIGI_is_odd);
                    assert(is_a == is_b);
                    break;
                }
                case TEST_COUNT:
                {
                    int key = TEST_RAND(TEST_MAX_SIZE);
                    int aa = vec_digi_count(&a, digi_init(key));
                    int bb = std::count(b.begin(), b.end(), DIGI{key});
                    assert(aa == bb);
                    break;
                }
                case TEST_COUNT_IF:
                {
                    size_t count_a = vec_digi_count_if(&a, digi_is_odd);
                    size_t count_b = std::count_if(b.begin(), b.end(), DIGI_is_odd);
                    assert(count_a == count_b);
                    break;
                }
            }
            CHECK(a, b);
            vec_digi_free(&a);
        }
    }
    if (fail)
        TEST_FAIL(__FILE__);
    else
        TEST_PASS(__FILE__);
}
