#ifndef UTILS_H
#define UTILS_H

#define FAST_RAND_MAX 32767

double getUnixTime(void);
void fast_srand(int seed);
int fast_rand(void);
int fast_rand_seeded(int m_seed);
unsigned long hash(unsigned char *str);
double fastPow(double a, double b);
int ceil_log2(unsigned long long x);

#endif /* UTILS_H */
