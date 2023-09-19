#define _STATIC_ASSERT_H2(a, b) a##b
#define _STATIC_ASSERT_H1(c, l)                                                                                        \
    static void _STATIC_ASSERT_H2(_STATIC_ASSERT_LINE_, l)()                                                           \
    {                                                                                                                  \
        int a[1 - 2 * (!(c))];                                                                                         \
    }
// this will throw a negative array size when the assertion
//   fails at compile time; check line it was expanded from
#define STATIC_ASSERT(cond) _STATIC_ASSERT_H1(cond, __LINE__)
