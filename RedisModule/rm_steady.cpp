#include "rm_steady.h"

#define CALLOC(count, size) RedisModule_Calloc(count, size)
#define FREE(ptr) RedisModule_Free(ptr)

#define ERROR(x)                        \
    RedisModule_ReplyWithError(ctx, x); \
    return REDISMODULE_ERR;

typedef SteadySketch SKETCH;

static RedisModuleType *SKETCHType;

static int GetKey(RedisModuleCtx *ctx, RedisModuleString *keyName, SKETCH **sketch, int mode)
{
    RedisModuleKey *key = (RedisModuleKey*)RedisModule_OpenKey(ctx, keyName, mode);
    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY){
        RedisModule_CloseKey(key);
        ERROR("Steady: key does not exist");
    } 
    else if (RedisModule_ModuleTypeGetType(key) != SKETCHType){
        RedisModule_CloseKey(key);
        ERROR(REDISMODULE_ERRORMSG_WRONGTYPE);
    }

    *sketch = (SKETCH *)RedisModule_ModuleTypeGetValue(key);
    RedisModule_CloseKey(key);
    return REDISMODULE_OK;
}

static int create(RedisModuleCtx *ctx, RedisModuleString **argv, int argc, SKETCH **sketch)
{
    long long memory;
    if(RedisModule_StringToLongLong(argv[2], &memory) != REDISMODULE_OK){
        ERROR("Steady: invalid memory");
    }
    *sketch = (SKETCH*)CALLOC(1, sizeof(SKETCH));
    (*sketch)->init(memory);
    return REDISMODULE_OK;
}

static int Create_Cmd(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3){
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleKey *key = (RedisModuleKey *)RedisModule_OpenKey(ctx, argv[1],REDISMODULE_READ | REDISMODULE_WRITE);
    SKETCH *sketch = NULL;
    if(RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_EMPTY){
        RedisModule_ReplyWithError(ctx, "Steady: key already exists");
        goto final;
    }

    if(create(ctx, argv, argc, &sketch) != REDISMODULE_OK){
        goto final;
    }

    if (RedisModule_ModuleTypeSetValue(key, SKETCHType, sketch) == REDISMODULE_ERR)
    {
        goto final;
    }

    RedisModule_ReplicateVerbatim(ctx);
    RedisModule_ReplyWithSimpleString(ctx, "OK");
final:
    RedisModule_CloseKey(key);
    return REDISMODULE_OK;

}

static int Insert_Cmd(RedisModuleCtx *ctx, RedisModuleString **argv,int argc){
    if(argc < 3)
        return RedisModule_WrongArity(ctx);
    
    SKETCH *sketch;
    if(GetKey(ctx, argv[1], &sketch, REDISMODULE_READ | REDISMODULE_WRITE) != REDISMODULE_OK)
        return REDISMODULE_ERR;
    
    // long long time;

    // if ((RedisModule_StringToLongLong(argv[2], &time) != REDISMODULE_OK) || time < 0)
    // {
    //     ERROR("Steady: invalid time");
    // }

    std::vector<std::pair<uint32_t, std::pair<const char*,size_t>>> result_per_time;
    int time = 0;
    
    int itemCount = argc -2;
    long long res = 0;
    // RedisModule_ReplyWithArray(ctx, itemCount * 2);
    for(int i = 2; i < argc; ++i){
        size_t itemlen;
        const char* item = RedisModule_StringPtrLen(argv[i], &itemlen);
        time = i - 2;
        res = sketch->insert(item, itemlen, time);
        if (res){
            result_per_time.push_back(std::make_pair(time / 10000,std::make_pair(item, itemlen)));
        }
        // RedisModule_ReplyWithStringBuffer(ctx, item, itemlen);
        // RedisModule_ReplyWithLongLong(ctx, res);
        if((time > 0) && (time % 10000 == 0))
            sketch->clear(time);
    }

    // for(int i = 0; i < sketch->pers._NumOfHeavyHitterBuckets; ++i){
    //     for(int j = 0; j < ElementPerBucket;++j){
    //         if(sketch->pers.HeavyHitter[i][j].RecentStableTime - sketch->pers.HeavyHitter[i][j].StartStableTime > SmoothThreshold){
    //             result_per_time.push_back(std::make_pair(sketch->pers.HeavyHitter[i][j].RecentStableTime, std::make_pair(sketch->pers.HeavyHitter[i][j].ItemID, sketch->pers.HeavyHitter[i][j].itemlen)));
    //         }
    //     }
    // }

    int result_len = result_per_time.size();
    RedisModule_ReplyWithArray(ctx, result_len * 2);
    for(auto i = result_per_time.begin(); i != result_per_time.end(); i++){
        RedisModule_ReplyWithLongLong(ctx, (long long)i->first);
        RedisModule_ReplyWithStringBuffer(ctx, i->second.first, i->second.second);
    }
    result_per_time.clear();

    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}

static void RdbSave(RedisModuleIO *io, void *obj){
    SKETCH *sketch = (SKETCH *)obj;
    RedisModule_SaveSigned(io, sketch->filter_hash);
    RedisModule_SaveSigned(io, sketch->rollling_hash);

    RedisModule_SaveSigned(io, sketch->steadyfilter.array_num);
    RedisModule_SaveSigned(io, sketch->steadyfilter.counter_num);
    for(int i = 0; i < sketch->steadyfilter.array_num; i++){
        RedisModule_SaveStringBuffer(io, (const char*)sketch->steadyfilter.counters[i], sketch->steadyfilter.array_num * sizeof(uint8_t));
    }

    RedisModule_SaveSigned(io, sketch->rollingsketch.array_num);
    RedisModule_SaveSigned(io, sketch->rollingsketch.counter_num);
    for(int i = 0; i < sketch->rollingsketch.array_num; i++){
        for(int j = 0; j < sketch->rollingsketch.counter_num; j++){
            RedisModule_SaveStringBuffer(io, (const char*)sketch->rollingsketch.Rolling[i][j], (_Period + 2) * sizeof(uint8_t));
        }
    }

    // RedisModule_SaveSigned(io, sketch->pers._NumOfHeavyHitterBuckets);
    // for(int i = 0; i < sketch->pers._NumOfHeavyHitterBuckets; ++i){
    //     RedisModule_SaveStringBuffer(io, (const char*)sketch->pers.HeavyHitter[i], ElementPerBucket * sizeof(StableElement));
    // }
    // for(int i = 0; i < sketch->pers._NumOfHeavyHitterBuckets; ++i){
    //     for(int j = 0; j < ElementPerBucket; ++j){
    //         if(sketch->pers.HeavyHitter[i][j].itemlen != 0)
    //         RedisModule_SaveStringBuffer(io, (const char*)sketch->pers.HeavyHitter[i][j].ItemID, sketch->pers.HeavyHitter[i][j].itemlen * sizeof(char));
    //     }
    // }
}

static void *RdbLoad(RedisModuleIO *io, int encver){
    if(encver > STEADY_ENC_VER){
        return NULL;
    }

    size_t tmp;

    SKETCH *sketch = (SKETCH *)CALLOC(1, sizeof(SKETCH));
    sketch->filter_hash = RedisModule_LoadSigned(io);
    sketch->rollling_hash = RedisModule_LoadSigned(io);

    sketch->steadyfilter.array_num = RedisModule_LoadSigned(io);
    sketch->steadyfilter.counter_num = RedisModule_LoadSigned(io);
    for(int i = 0; i < sketch->steadyfilter.array_num; ++i)
        sketch->steadyfilter.counters[i] =(uint8_t*) RedisModule_LoadStringBuffer(io, &tmp);

    sketch->rollingsketch.array_num = RedisModule_LoadSigned(io);
    sketch->rollingsketch.counter_num = RedisModule_LoadSigned(io);
    for(int i = 0; i < sketch->steadyfilter.array_num; ++i)
        for(int j = 0; j < sketch->steadyfilter.counter_num; ++j)
            sketch->rollingsketch.Rolling[i][j] = (uint8_t*)RedisModule_LoadStringBuffer(io, &tmp);

    // sketch->pers._NumOfHeavyHitterBuckets = RedisModule_LoadSigned(io);
    // for(int i = 0; i < sketch->pers._NumOfHeavyHitterBuckets; ++i){
    //     sketch->pers.HeavyHitter[i] =(StableElement*)RedisModule_LoadStringBuffer(io, &tmp);
    // }
    // for(int i = 0; i < sketch->pers._NumOfHeavyHitterBuckets; ++i){
    //     for(int j = 0; j < ElementPerBucket; ++j){
    //         if(sketch->pers.HeavyHitter[i][j].itemlen != 0)
    //             sketch->pers.HeavyHitter[i][j].ItemID = (char*)RedisModule_LoadStringBuffer(io, &tmp);
    //     }
    // }

    
}

static void Free(void *value){
    SKETCH *sketch = (SKETCH*)value;
    sketch->destroy();
    FREE(sketch);
}

int SteadyModule_onLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc){
    RedisModuleTypeMethods tm = {.version = REDISMODULE_TYPE_METHOD_VERSION,
                                .rdb_load = RdbLoad,
                                .rdb_save = RdbSave,
                                .aof_rewrite = RMUtil_DefaultAofRewrite,
                                .free = Free};
    SKETCHType = RedisModule_CreateDataType(ctx, "STEADY_SK", STEADY_ENC_VER, &tm);
    if(SKETCHType == NULL){
        return REDISMODULE_ERR;
    }

    RMUtil_RegisterWriteDenyOOMCmd(ctx, "steady.create", Create_Cmd);
    RMUtil_RegisterWriteDenyOOMCmd(ctx, "steady.insert", Insert_Cmd);
    
    return REDISMODULE_OK;
}