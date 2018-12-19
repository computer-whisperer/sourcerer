#include <time.h>

double getUnixTime(void)
{
    struct timespec tv;

    if(clock_gettime(CLOCK_REALTIME, &tv) != 0) return 0;

    return (tv.tv_sec + (tv.tv_nsec / 1000000000.0));
}

// fast random number generated yanked from the internet

unsigned int g_seed;

// Used to seed the generator.           
void fast_srand(int seed) {
    g_seed = seed;
}

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}

// Compute a pseudorandom integer.
// Output value in range [0, 32767]
int fast_rand_seeded(int m_seed) {
    m_seed = (214013*m_seed+2531011);
    return (m_seed>>16)&0x7FFF;
}

