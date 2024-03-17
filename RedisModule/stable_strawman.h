#include "basic_sketch/basic_sketch.h"

#include <assert.h>

#ifndef _STEADY_STRAWMAN_H_
#define _STEADY_STRAWMAN_H_
#include <bits/stdc++.h>
#include "param.h"
// #include "Common/hash.h"
#include "basic_smooth/Common/hash.h"
#include "basic_smooth/baselilne.h"
#include "stage2.h"


#define inf 2147483647
#define eps 1e-6



// template<typename ID_TYPE>
// class CountMinSketch{
// private:

// public:
// 	uint32_t **CMSketch;
// 	uint32_t CounterOfArray;

// 	CountMinSketch(){}

//     void init(uint32_t memory){
// 		CounterOfArray = memory * 1024 / (_NumOfArray * sizeof(uint32_t));
// 		// std::cout<<"counter of each array: "<<CounterOfArray<<std::endl;
// 		// CMSketch = new uint32_t*[_NumOfArray];
//         CMSketch = (uint32_t**)CALLOC(_NumOfArray, sizeof(uint32_t*));
// 		for(int i = 0; i < _NumOfArray; ++i){
// 			// CMSketch[i] = new uint32_t[CounterOfArray]();
//             CMSketch[i] = (uint32_t *)CALLOC(CounterOfArray, sizeof(uint32_t));
//             memset(CMSketch[i], 0, CounterOfArray * sizeof(uint32_t));
// 		}
//     }
//     ~CountMinSketch(){}

// 	void destroy(){
// 		for(int i= 0; i < _NumOfArray; ++i){
// 			FREE(CMSketch[i]);
// 		}
// 		FREE(CMSketch);
// 	}

// 	int insert(ID_TYPE ItemID){
// 		for(int i = 0; i < _NumOfArray; ++i){
// 			uint32_t HashValue = hash_s(ItemID,i);
// 			// uint32_t HashValue = MurmurHash32((const void*)ItemID.data(),KEY_LEN, 50 + i);
// 			uint32_t loc = HashValue % CounterOfArray;
// 			// std::cout<<loc<<","<<CounterOfArray<<std::endl;
// 			CMSketch[i][loc] ++;
// 		}
// 		return 1;
// 	}

// 	uint32_t query(ID_TYPE ItemID){
// 		uint32_t freq_min = UINT32_MAX;
// 		for(int i = 0; i < _NumOfArray; ++i){
// 			uint32_t loc = hash_s(ItemID, i) % CounterOfArray;
// 			// uint32_t loc = MurmurHash32((const void*)ItemID.data(), KEY_LEN, 50 + i) % CounterOfArray;
// 			uint32_t freq_loc = CMSketch[i][loc];
// 			freq_min = std::min(freq_min,freq_loc);
// 		}
// 		return freq_min;
// 	}

// 	void clear(){
// 		for(int i = 0; i < _NumOfArray; ++i){
// 			memset(CMSketch[i], 0, CounterOfArray * sizeof(uint32_t));
// 		}
// 	}

// };

typedef long long ID_TYPE;

class Steady_CM: public basic_sketch{
private:
	int NumOfSketch = _Period + 2;
	CountMinSketch<ID_TYPE>* Basic[_Period + 2];
	// per_stage pers;
	std::vector<std::pair<ID_TYPE, uint32_t>> result;
	uint32_t CountersNum;
	uint32_t cur_timestamp;
	uint32_t last_timestamp;
	uint32_t win_cnt;
public:
	using basic_sketch::operator new;
    using basic_sketch::operator new[];
    using basic_sketch::operator delete;
    using basic_sketch::operator delete[];

	Steady_CM(){}

	Steady_CM(int argc, basic_sketch_string *argv){
		cur_timestamp = 0;
		last_timestamp = 0;
		win_cnt = 0;
		int memory = argv[0].to_int();
		int MemPerSketch = memory/NumOfSketch;

		CountersNum = MemPerSketch * 1024/ (_ArrayNum * sizeof(uint32_t));

		for(int i = 0; i < NumOfSketch; ++i){
			Basic[i] = (CountMinSketch<ID_TYPE>*)CALLOC(1, sizeof(CountMinSketch<ID_TYPE>));
			Basic[i]->init(MemPerSketch);
		}

		// pers.init(200);

	}
	~Steady_CM(){
		for(int i =0; i < NumOfSketch; i++){
			FREE(Basic[i]);
		}
		// pers.destroy();
	}

	Steady_CM(const basic_sketch_string &s){
		size_t tmp = 0;
		const char *ss = s.c_str();

		memcpy(&last_timestamp, ss + tmp, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		memcpy(&win_cnt, ss + tmp, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		memcpy(&CountersNum, ss + tmp, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		memcpy(&cur_timestamp, ss + tmp, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		for(int i = 0; i < S; ++i){
			Basic[i]->CounterOfArray = CountersNum;
			for(int j = 0; j < _NumOfArray; ++j){
				memcpy(&Basic[i]->CMSketch[j], ss + tmp, CountersNum * sizeof(uint32_t));
				tmp += CountersNum * sizeof(uint32_t);
			}
		}

		// memcpy(&pers._NumOfHeavyHitterBuckets, ss + tmp, sizeof(int));
		// tmp += sizeof(int);
		// for(int i = 0; i < pers._NumOfHeavyHitterBuckets; i++){
		// 	memcpy(pers.HeavyHitter[i], ss + tmp, ElementPerBucket * sizeof(StableElement));
		// 	tmp += ElementPerBucket * sizeof(StableElement);
		// }
		// for(int i = 0; i < pers._NumOfHeavyHitterBuckets; i++){
		// 	for(int j = 0; j < ElementPerBucket; j++){
		// 		memcpy(pers.HeavyHitter[i][j].ItemID, ss + tmp, pers.HeavyHitter[i][j].itemlen * sizeof(char));
		// 		tmp += pers.HeavyHitter[i][j].itemlen * sizeof(char);
		// 	}
		// }
	}

	basic_sketch_string *to_string(){
		char *s1 = (char*)CALLOC(4 + CountersNum * _NumOfArray * P, sizeof(uint32_t));
		size_t tmp = 0;

		memcpy(s1 + tmp, &last_timestamp, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		memcpy(s1 + tmp, &win_cnt, sizeof(uint32_t));
		tmp += sizeof(uint32_t);
	
		memcpy(s1 + tmp, &CountersNum, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		memcpy(s1 + tmp, &cur_timestamp, sizeof(uint32_t));
		tmp += sizeof(uint32_t);

		for(int i = 0; i < P; ++i){
			for(int j = 0; j < _NumOfArray; ++j){
				memcpy(s1 + tmp, &Basic[i]->CMSketch[j], CountersNum * sizeof(uint32_t));
				tmp += CountersNum * sizeof(uint32_t);
			}
		}

		// memcpy(s1 + tmp, &pers._NumOfHeavyHitterBuckets, sizeof(int));
		// tmp += sizeof(int);

		// for(int i = 0; i < pers._NumOfHeavyHitterBuckets; i++){
		// 	memcpy(s1+tmp, &pers.HeavyHitter[i], ElementPerBucket * sizeof(StableElement));
		// 	tmp += ElementPerBucket * sizeof(StableElement);
		// }
		// for(int i = 0; i < pers._NumOfHeavyHitterBuckets; i++){
		// 	for(int j = 0; j < ElementPerBucket; j++){
		// 		memcpy(s1 + tmp, &pers.HeavyHitter[i][j].ItemID, pers.HeavyHitter[i][j].itemlen * sizeof(char));
		// 		tmp += pers.HeavyHitter[i][j].itemlen * sizeof(char);
		// 	}
		// }

		basic_sketch_string *bs = new basic_sketch_string(s1, tmp);

		return bs;
	}

	basic_sketch_reply *insert(int argc, const basic_sketch_string *argv){
		basic_sketch_reply *insert_out = new basic_sketch_reply;

		for(int i = 0; i < argc; ++i){
			int flag = 0;
			basic_sketch_string id = argv[i];
			long long itemid = id.to_long_long();
			cur_timestamp ++;
			win_cnt = cur_timestamp / windows_size;
			Basic[win_cnt % (_Period + 2)]->insert(itemid);
			uint32_t freq[_Period];
			memset(freq, 0xffffffff, sizeof(uint32_t) * _Period);
			int k = _Period - 1;
			for(uint32_t i = 2; i < _Period + 2; ++i){
				uint32_t freq_k = Basic[win_cnt % (_Period + 2)]->query(itemid);
				freq[k] = MIN(freq[k], freq_k); 
			}

			if(i > 0 && i % 10000 == 0)
				Basic[win_cnt % (_Period + 2)]->clear();

			int sum = 0;
			for(int i = 0; i < _Period; i++){
				sum += freq[i];
			}
			int ave = 0;
			ave = sum / _Period;
			int DX = 0;
			for(int i = 0; i < _Period; i++){
				DX += (freq[i] - ave) * (freq[i] - ave);
			}

			if(DX < _Period * _DX_thres){
				
				// flag = pers.insert(id.c_str(), 8, win_cnt);
				// if(flag) 
					result.push_back(std::make_pair(itemid, win_cnt));
			}
		}

		// for(int i = 0; i < pers._NumOfHeavyHitterBuckets; ++i){
        // for(int j = 0; j < ElementPerBucket;++j){
        //     if(pers.HeavyHitter[i][j].RecentStableTime -pers.HeavyHitter[i][j].StartStableTime > SmoothThreshold){
		// 		basic_sketch_string* iid = new basic_sketch_string(pers.HeavyHitter[i][j].ItemID, 8);
		// 		long long iiid =iid->to_long_long();
        //         result.push_back(std::make_pair(iiid, pers.HeavyHitter[i][j].RecentStableTime));
        //     	}
        // }
    	// }

		for(auto it = result.begin(); it != result.end(); it++){
			insert_out->push_back(it->first);
			insert_out->push_back((long long)it->second);
		}
		return insert_out;
	}

	static basic_sketch_reply *Insert(void *o, const int &argc, const basic_sketch_string *argv){
		return ((Steady_CM*)o)->insert(argc, argv);
	}

	static int command_num(){return 1;}

	static basic_sketch_string command_name(int index){
		basic_sketch_string tmp[] = {"insert"};
		return tmp[index];
	}

	static basic_sketch_func command(int index){
		basic_sketch_func tmp[] = {(Steady_CM::Insert)};
		return tmp[index];
	}

	static basic_sketch_string class_name(){return "SteadyCM";}

	static int command_type(int index){
		int tmp[] = {0};
		return tmp[index];
	}

	static char* type_name(){return "STEADY_CM";}
};

#endif