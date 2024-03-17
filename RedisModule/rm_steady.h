#ifndef _STEADY_MODULE_H_
#define _STEADY_MODULE_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "util.h"
#ifdef __cplusplus
};
#endif

#include "util/redismodule.h"
#include "util/version.h"
#include "steady.h"

#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#define STEADY_ENC_VER 0
#define REDIS_MODULE_TARGET

int SteadyModule_onLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc);

#endif