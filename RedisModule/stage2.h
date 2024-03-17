#ifndef _ROLLING_SKETCH_H_
#define _ROLLING_SKETCH_H_

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


class RollingSketch
{
private:
    /* data */
public:
    int array_num;
    int counter_num;
    uint8_t ***Rolling;
    RollingSketch(){};
    ~RollingSketch(){};
    void init(int memory, int hash_num){
        array_num = hash_num;
        counter_num = memory/(array_num * (_Period + 2));

        Rolling = (uint8_t ***)CALLOC(array_num, sizeof(uint8_t**));
        for(int i = 0; i < array_num; ++i){
            Rolling[i] = (uint8_t**)CALLOC(counter_num, sizeof(uint8_t*));
            for(int j = 0; j < counter_num; ++j){
                Rolling[i][j] = (uint8_t*)CALLOC(_Period + 2, sizeof(uint8_t));
                memset(Rolling[i][j], 0, sizeof(uint8_t) * (_Period + 2));
            }
        }
    }

    int insert(const char* key, size_t key_len, size_t time, int flag){
        int win_cnt = time % (_Period + 2);
        uint8_t freq[_Period];
        // memset(freq, 0xff, _Period * sizeof(uint32_t));
        for(int i = 0; i < _Period; i++){
            freq[i] = 255;
        }

        for(int i = 0; i < array_num; ++i){
            uint32_t hash_val = HASH(key, key_len, 100 + i) % counter_num;
            Rolling[i][hash_val][win_cnt]++;

            if(flag == 1){
                int k = (_Period - 1);
                for(uint32_t j = 2; j < (_Period + 2); ++j){
                    freq[k] = MIN(freq[k], Rolling[i][hash_val][win_cnt + j]);
                    k--;
                }
            }

        }

        if(flag == 1){
            int sum1 = 0;
            int sum2 = 0;
            int ave1 = 0;
            int ave2 = 0;
            for(int i = 0; i < _Period; i ++){
                sum1 += freq[i];
                sum2 += freq[i] + 128;
            }
            ave1 = sum1/_Period;
            ave2 = sum2/_Period;
            int DX1 = 0;
            int DX2 = 0;
            for(int i = 0; i < _Period; i++){
                DX1 += (freq[i] - ave1) * (freq[i] - ave1);
                DX2 += (freq[i] + 128 - ave2) * (freq[i] + 128 - ave2);
            }
            if((DX1 < _DX_thres * _Period) || (DX2 < _DX_thres * _Period))
                return 1;
        }

        return 0;
    }

    void clear(int time){
        for(int i = 0; i < array_num; ++i){
            for(int j = 0; j < counter_num; ++j){
                Rolling[i][j][(time + 1) % (_Period + 2)] = 0;
            }
        }
    }

    
};


struct StableElement {		//Like Unbiased Space Saving. We use the StableElement to save the current stable item.

	int StartStableTime = 0;	//To save the first time the item is stable.

	int RecentStableTime = 0;	//To save the last time the item is stable.

	char* ItemID = NULL;	//To save the item itself.

    int itemlen = 0;
};

class per_stage{

public:
    StableElement** HeavyHitter;
    int _NumOfHeavyHitterBuckets;
    per_stage(){}
    ~per_stage(){}
    void init(int memory){
        // _NumOfHeavyHitterBuckets = memory * 1024 / (ElementPerBucket * sizeof(StableElement));
        _NumOfHeavyHitterBuckets = 10000;
        
        HeavyHitter = (StableElement**)CALLOC(_NumOfHeavyHitterBuckets, sizeof(StableElement*));
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			HeavyHitter[i] = (StableElement*)CALLOC(ElementPerBucket, sizeof(StableElement));
		}
    }

    void destroy(){
        for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			FREE(HeavyHitter[i]);
		}
        FREE(HeavyHitter);
    }

    int insert(const char* key_c, int KEY_LEN, int time){

            char* key = (char*)key_c;

            int HashBucket = HASH(key, KEY_LEN, 100) % _NumOfHeavyHitterBuckets;

			int LeastSmoothIndex = 0, LeastSmoothTime = INT_MAX;	//If the bucket is full, we should kick the least smooth item with certain probability.
			
			for (int i = 0; i < ElementPerBucket; i++) {

				StableElement* e_ = &HeavyHitter[HashBucket][i];

				if (e_->ItemID == NULL) {	//Case 1: If it is an empty slot. Insert the coming item here.
					e_->StartStableTime = time;
					e_->RecentStableTime = time + 1;
					e_->ItemID = key;
                    e_->itemlen = KEY_LEN;
					return 0;
				}
				if (e_->RecentStableTime < time) {		//Case 2: If the stable element is interrupted, we can safely replace it.

					//LXD ---chenge ItemID format
					// ItemID_1 = changeID(e_->ItemID);

					if (e_->RecentStableTime - e_->StartStableTime > SmoothThreshold) {
                        // ReportBuffer.push_back(make_pair(ItemID_1, make_pair(e_->StartStableTime, e_->RecentStableTime))); 	//If the item is stable for more than SmoothThreshold
                        return 1;
                    }																						//times continuously, we will report this value.
					//LXD --end

					e_->StartStableTime =time;
					e_->RecentStableTime = time + 1;

					e_->ItemID = key;//Replace.
                    e_->itemlen = KEY_LEN;

					return 0;
				}

				if (memcmp(e_->ItemID, key, KEY_LEN) == 0) {		//Case 3: If it is an existing item, we increase the corresponding RecentStableTime.

					e_->RecentStableTime = time + 1;

					return 0;
				}

				if (e_->RecentStableTime - e_->StartStableTime < LeastSmoothTime) {	//Case 4: If Case 1 to 3 are not satisfied. We should find the least smooth item.
																					//and replace it with certaion probability.

					LeastSmoothTime = e_->RecentStableTime - e_->StartStableTime;

					LeastSmoothIndex = i;
				}
			}

			HeavyHitter[HashBucket][LeastSmoothIndex].RecentStableTime =time + 1;		//No matter the original is kicked or not,
																							//we increase the RecentStableTime by 1.

			int Random = rand();

			if ((LeastSmoothTime + 1) * Random <= RAND_MAX){
                HeavyHitter[HashBucket][LeastSmoothIndex].ItemID = key;
                HeavyHitter[HashBucket][LeastSmoothIndex].itemlen = KEY_LEN;
            }
			//Kick the original item with probability 1/(fx+1).Where fx is 
			//the frequency of the least frequent element in the bucket.
			//1/(fx+1) can lead to an unbiased estimation by USS (SIGMOD2018).
		
    }


};



#endif