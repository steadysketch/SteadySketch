#ifndef SteadySketch_H
#define SteadySketch_H

#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include <vector>
#include <map>
#include <unordered_map>

using namespace std;

class SteadySketch {
private:
	short* RBF;
	short* RBF_test;
	unsigned char* GS;
	unsigned d, L, RBFNum, RBFL, hash_seed;
	const unsigned w = _Period + 2, longdivshort = (int)sizeof(long long) / sizeof(short), RBFalpha = 10, GSalpha = 10;
	const unsigned long long RBFmod = 0x0001000100010001;
	unsigned TimeStamp;
	StableElement** HeavyHitter;
	//LXD
	string ItemID_1;
	string ItemID_2;
	int DX_pre = 0;
	int DX_cur = 0;
	int stable_len = 0;
	
public:
	vector<pair<char*,  pair<int, int>>> ReportBuffer = {};		
	//To report the smooth item. ReportBuffer contains the smooth items.
	multimap<string, pair<int, pair<int, int>>> TopKReport = {};
	double RBF_size = 0;
	double GS_size = 0;
	double USS_size = 0; 
	SteadySketch(int _d, int _L, int _RBFL, int _RBFNum , int _hash_seed = 100) {
		d = _d;
		L = _L;
		RBFNum = _RBFNum;
		RBFL = _RBFL;
		hash_seed = _hash_seed;
		RBF = new short[RBFNum*RBFL]();
		GS = new unsigned char[d * L * w]();
		TimeStamp = 0;
		HeavyHitter = new StableElement * [_NumOfHeavyHitterBuckets];
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			HeavyHitter[i] = new StableElement[ElementPerBucket]();
		}
		RBF_size = double(sizeof(short) * RBFNum * RBFL)/1024;
		GS_size = double(sizeof(unsigned char) * d * L * w)/1024;
		USS_size = double(sizeof(StableElement) * _NumOfHeavyHitterBuckets * ElementPerBucket)/1024;
		
	}
	void Insert(Item* e, int KEY_LEN) {

		int LastTimeStamp = TimeStamp;
		TimeStamp = e->Window % w;
		int NextStamp = TimeStamp + 1;
		if (NextStamp == w)NextStamp = 0;

		if (TimeStamp != LastTimeStamp) {
			long long* p = (long long*)RBF;
			int step = RBFNum * RBFL / longdivshort;
			for (int i = 0; i < step; i++) p[i] &= (~(RBFmod << NextStamp));

			//LXD-- clear all next time window buckets
			unsigned GS_len = d * L * w;
			int step_gs = NextStamp;
			while (step_gs < GS_len)
			{
				GS[step_gs] = 0;
				step_gs += w;
			}
			
		}

		uint64_t HashValue = MurmurHash64B((const void*)e->ItemID, KEY_LEN, hash_seed);

		int TagRBF = 0;
		unsigned long long hashvalue = HashValue;
		unsigned tmp = (((RBFL >> RBFalpha) - 1) << RBFalpha);
		if(tmp == 0) tmp = 1;
		unsigned index = (hashvalue >> (RBFNum * RBFalpha)) % tmp;
		for (int i = 0; i < RBFNum; i++) {
			unsigned addr = (index + (hashvalue & ((1 << RBFalpha) - 1))) % ((i+1) * RBFL);
			TagRBF += (RBF[addr] & (1 << TimeStamp)) >> TimeStamp;
			RBF[addr] |= 1 << TimeStamp;
			if (((~(RBF[addr] | (1 << NextStamp))) & ((1 << w) - 1)) != 0)TagRBF = RBFNum;
			index += RBFL;
			hashvalue >>= RBFalpha;
		}
		hashvalue = HashValue;
		unsigned tmp_1 = (((L >> GSalpha) - 1) << GSalpha);
		if(tmp_1 == 0) tmp_1 = 1; 
		index = (hashvalue >> (d * GSalpha)) % tmp_1;
		unsigned char Group[2][sizeof(short)<<3] = {};

		for (int i = 0; i < w; i++) Group[0][i] = Group[1][i] = 0xff;

		for (int i = 0; i < d; i++) {
			unsigned addr = ((index + (hashvalue & ((1 << GSalpha) - 1))) % ((i+1)*L)) * w;
			GS[addr + TimeStamp]++;
			GS[addr + NextStamp] = 0;

			index += L;
			hashvalue >>= GSalpha;
			if (TagRBF >= RBFNum)continue;
			for (int j = 0; j < w; j++) {
				Group[0][j] = Min(Group[0][j], GS[addr + j]);
				Group[1][j] = Min(Group[1][j], GS[addr + j] + (unsigned char)0x80);
			}
			Group[0][TimeStamp] = Group[1][TimeStamp] = 0;
		}
		if (TagRBF >= RBFNum)return;

		int EX[2] = {}, EX2[2] = {};

		for (int i = 0; i < w; i++) {
			EX[0] += (unsigned)Group[0][i];
			EX2[0] += ((unsigned)Group[0][i]) * Group[0][i];

			EX[1] += (unsigned)Group[1][i];
			EX2[1] += ((unsigned)Group[1][i]) * Group[1][i];
		}


		if (EX2[0] * (w - 2) - EX[0] * EX[0] < StableThreshold * (w - 2) * (w - 2) ||
			EX2[1] * (w - 2) - EX[1] * EX[1] < StableThreshold * (w - 2) * (w - 2)) {	//If DX>StableThreshold. We say this item is stable.

			// LXD DX calculating
			if(EX2[0] * (w - 2) - EX[0] * EX[0] < StableThreshold * (w - 2) * (w - 2)){
				DX_cur = EX2[0] * (w - 2) - EX[0] * EX[0];
			}
			else {
				DX_cur = EX2[1] * (w - 2) - EX[1] * EX[1];
			}		

			unsigned HashBucket = HashValue % _NumOfHeavyHitterBuckets;

			int LeastSmoothIndex = 0, LeastSmoothTime = INT_MAX;	//If the bucket is full, we should kick the least smooth item with certain probability.

			for (int i = 0; i < ElementPerBucket; i++) {

				StableElement* e_ = &HeavyHitter[HashBucket][i];

				if (e_->ItemID == NULL) {	//Case 1: If it is an empty slot. Insert the coming item here.
					e_->StartStableTime = e->Window;
					e_->RecentStableTime = e->Window + 1;
					e_->ItemID = e->ItemID;
					//LXD
					return;
				}
				if (e_->RecentStableTime < e->Window) {		//Case 2: If the stable element is interrupted, we can safely replace it.


					if (e_->RecentStableTime - e_->StartStableTime > SmoothThreshold){
						//LXD
						stable_len = e_->RecentStableTime - e_->StartStableTime;
						//e_->DX_sum = e_->DX_sum/stable_len;
						ReportBuffer.push_back(make_pair(e_->ItemID, make_pair(e_->StartStableTime, e_->RecentStableTime))); 	//If the item is stable for more than SmoothThreshold
					}																							//times continuously, we will report this value.

					e_->StartStableTime = e->Window;
					e_->RecentStableTime = e->Window + 1;

					e_->ItemID = e->ItemID;//Replace.

					return;
				}

				if (memcmp(e_->ItemID, e->ItemID, KEY_LEN) == 0) {		//Case 3: If it is an existing item, we increase the corresponding RecentStableTime.

					e_->RecentStableTime = e->Window + 1;

					return;
				}

				if (e_->RecentStableTime - e_->StartStableTime < LeastSmoothTime) {	//Case 4: If Case 1 to 3 are not satisfied. We should find the least smooth item.
																					//and replace it with certaion probability.

					LeastSmoothTime = e_->RecentStableTime - e_->StartStableTime;

					LeastSmoothIndex = i;
				}
			}


			HeavyHitter[HashBucket][LeastSmoothIndex].RecentStableTime = e->Window + 1;		//No matter the original is kicked or not,
																							//we increase the RecentStableTime by 1.

			int Random = rand();

			if ((LeastSmoothTime + 1) * Random <= RAND_MAX)HeavyHitter[HashBucket][LeastSmoothIndex].ItemID = e->ItemID;

		}



	}

	void Flush() {
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			for (int j = 0; j < ElementPerBucket; j++) {
				if (HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime > SmoothThreshold)
				{

					stable_len = HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime;
					ReportBuffer.push_back(make_pair(HeavyHitter[i][j].ItemID, 
						make_pair(HeavyHitter[i][j].StartStableTime, HeavyHitter[i][j].RecentStableTime)));
				}
			}
		}
	}

	void ReportValue(double Throughput) {		//Report the smooth items.
		sort(ReportBuffer.begin(), ReportBuffer.end(), compare);
		int k = 0;
		int stablelength = 0;
		for (auto i = ReportBuffer.begin(); i != ReportBuffer.end(); i++) {
			ItemID_2 = changeID(i->first);
			TopKReport.insert(make_pair(ItemID_2, make_pair(i->second.second - i->second.first,
				make_pair(i->second.first, i->second.second))));//
			if (k >= TopK - 1 && stablelength != i->second.second - i->second.first)break;//
			k++, stablelength = i->second.second - i->second.first;//

		}

		printf("GroupSketch reports %u Top %d smooth items and the throughput is %lf Kibs.", (unsigned)TopKReport.size(), TopK, Throughput);
	}
	~SteadySketch() {
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			delete[] HeavyHitter[i];
		}
		delete[] GS;
		delete HeavyHitter;
	}
};

#endif