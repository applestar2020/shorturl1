#ifndef __NGX_MURHASH_
#define __NGX_MURHASH_

#include <iostream>


using namespace std;

unsigned int MurmurHash2(const void *key, int len, unsigned int seed);
string to_62(unsigned int x);

#endif


