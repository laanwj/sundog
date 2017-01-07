#ifndef H_UTIL_MINMAX
#define H_UTIL_MINMAX

/* Straightforward minimum and maximum functions */
static inline unsigned umin(unsigned a, unsigned b)
{
    return (a < b) ? a : b;
}

static inline unsigned umax(unsigned a, unsigned b)
{
    return (a > b) ? a : b;
}

static inline int imin(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int imax(int a, int b)
{
    return (a > b) ? a : b;
}

#endif
