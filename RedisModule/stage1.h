#ifndef _STEADT_FILTER_H_
#define _STEADT_FILTER_H_

#include "param.h"

#define REDIS_MODULE_TARGET
#ifdef REDIS_MODULE_TARGET
#include "util/redismodule.h"
#define CALLOC(count, size) RedisModule_Calloc(count, size)
#define FREE(ptr) RedisModule_Free(ptr)
#else
#define CALLOC(count, size) calloc(count, size)
#define FREE(ptr) free(ptr)
#endif

#define HASH(key, keylen, i) MurmurHash2(key, keylen, i)
#define MIN(x, y) ((x) < (y) ? (x) : (y))

class SteadyFilter
{
private:
    /* data */
public:
    int counter_num;
    uint8_t **counters;
    int array_num;

    SteadyFilter(){};
    ~SteadyFilter(){};
    
    void init(int memory, int hash_num){
        counter_num = memory / hash_num;
        array_num = hash_num;

        counters = (uint8_t **)CALLOC(hash_num, sizeof(uint8_t*));
        for(int i = 0; i < hash_num; ++i){
            counters[i] = (uint8_t *)CALLOC(counter_num, sizeof(uint8_t));
            memset(counters[i], 0, counter_num * sizeof(uint8_t));
        }

    }

    int insert(const char* key, size_t key_len, size_t time){
        int flag = 0;
        int win_cnt = time % _Period;
        int next_win = (time + 1) % _Period;
        int first_come = 0;
        int pers = 1;

        for(int i  = 0; i < array_num; ++i){
            uint32_t hash_val = HASH(key, key_len, 100 + i) % counter_num;
            uint8_t filter_val = counters[i][hash_val] | (1 - (1 << (_Period + 2)));
            uint8_t offset = 1 << win_cnt;
            first_come |= ~((filter_val & offset) >> win_cnt);
            counters[i][hash_val] |= offset;
            filter_val |= offset;
            filter_val |= (1 << next_win);
            if (filter_val < (1 - (1 << (_Period + 2))))
                pers = 0;
        }

        if(pers == 1 && first_come == 1){
            flag = 1;
        }

        return flag;
    }

    void clear(int time){
        for(int i = 0; i < array_num; ++i){
            for(int j = 0; j < counter_num; ++j){
                uint8_t offset =~(1 << ((time + 1) % (_Period + 2)));
                counters[i][j] &= offset;
            }
        }
    }
};



#endif