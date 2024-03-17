#include "steady.h"
#include "param.h"


#include <cstdio>

#define REDIS_MODULE_TARGET
#ifdef REDIS_MODULE_TARGET
#include "util/redismodule.h"
#define CALLOC(count, size) RedisModule_Calloc(count, size)
#define FREE(ptr) RedisModule_Free(ptr)
#else
#define CALLOC(count, size) calloc(count, size)
#define FREE(ptr) free(ptr)
#endif

void SteadySketch::init(int memory){
    int filter_mem = memory * ratio;
    int rolling_mem = memory * (1 - ratio);
    filter_hash = 3;
    rollling_hash = 2;
    steadyfilter.init(filter_mem, filter_hash);
    rollingsketch.init(rolling_mem, rollling_hash);
    // pers.init(200);
}

void SteadySketch::destroy(){
    for(int i = 0; i < filter_hash; ++i){
        FREE(steadyfilter.counters[i]);
    }
    FREE(steadyfilter.counters);

    for(int i = 0; i < rollling_hash; ++i){
        for(int j = 0; j < rollingsketch.counter_num; ++j){
            FREE(rollingsketch.Rolling[i][j]);
        }
        FREE(rollingsketch.Rolling[i]);
    }
    FREE(rollingsketch.Rolling);

    // for (int i = 0; i <pers._NumOfHeavyHitterBuckets; i++) {
	// 	FREE(pers.HeavyHitter[i]);
	// }
    // FREE(pers.HeavyHitter);

}

int SteadySketch::insert(const char* key, size_t key_len, int time){
    int flag = 0;
    uint32_t win_cnt = time / 10000; 
    flag = steadyfilter.insert(key, key_len, win_cnt);

    int res =0;
    res = rollingsketch.insert(key,key_len,win_cnt,flag);
    // if(res){
    //     res = pers.insert(key,key_len,win_cnt);
    // }
    
    return res;
}

void SteadySketch::clear(int time){
    steadyfilter.clear(time);
    rollingsketch.clear(time);
}

