#ifndef _STEADY_SKETCH_H
#define _STEADY_SKETCH_H

#include "util/MurmurHash2.h"

#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

#include "stage1.h"
#include "stage2.h"
#include "util/hash_table.h"

class SteadySketch
{
private:
    /* data */
public:
    SteadyFilter steadyfilter;
    RollingSketch rollingsketch;
    // per_stage pers;
    int filter_hash;
    int rollling_hash;
    SteadySketch(){};
    ~SteadySketch(){};
    void init(int memory);
    void destroy();
    void clear(int time);

    int insert(const char *key, size_t key_len, int time);
    int query();
};




#endif