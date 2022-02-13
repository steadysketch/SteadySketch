#ifndef Strawman_H
#define Strawman_H

#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include <vector>
#include <map>
#include <unordered_map>

using namespace std;




class CompareSketch {

private:

	StableElement** HeavyHitter;

	int* CountMinSketch[_SketchArray][_Period + 2] = {};

	int NumOfHeavyHitterBuckets;		//A HeavyHitter is NumOfHeavyHitterBuckets \times ElementPerBucket matrix of StableElements. HeavyHitter is
										//defined later.
	//LXD
	unsigned TimeStamp;
	string ItemID_1;
	string ItemID_2;
	int DX_cur = 0;
	int DX_pre = 0;
	int stable_len = 0;
	const unsigned CMalpha = 10;
    int _SketchSize = 0;


public:

	vector<pair<char*,  pair<int, int>>> ReportBuffer = {};		
	//To report the smooth item. ReportBuffer contains the smooth items.
	//<ItemID, <DX, <start, end>>>
	multimap<string, pair<int, pair<int, int>>> TopKReport = {};
	//<ItemID, <DX, <stable_len, <start, end>>>>
	double CM_size = 0;
	double USS_size = 0;

	CompareSketch(int sketchsize) {
        _SketchSize = sketchsize;
		NumOfHeavyHitterBuckets = _NumOfHeavyHitterBuckets;
		HeavyHitter = new StableElement * [_NumOfHeavyHitterBuckets];
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			HeavyHitter[i] = new StableElement[ElementPerBucket]();
		}
		for (int i = 0; i < _SketchArray; i++) {
			for (int j = 0; j < _Period + 2; j++) {
				CountMinSketch[i][j] = new int[_SketchSize]();
			}
		}
		CM_size = double(sizeof(int) * _SketchArray * _SketchSize *(_Period +2))/1024;
		USS_size = double(sizeof(StableElement)*NumOfHeavyHitterBuckets*ElementPerBucket)/1024;
		cout<<double(sizeof(int) * _SketchArray * _SketchSize *(_Period +2))/1024<<","<<USS_size<<endl;
	}

	void Insert(Item* e,int KEY_LEN) {

		//LXD clear all next time window buckets
		int LastTimeStamp = TimeStamp;
		TimeStamp = e->Window % (_Period + 2);

		if(LastTimeStamp != TimeStamp){
			for(int i = 0; i < _SketchArray; i++){
				for (int j = 0; j < _SketchSize; j ++)
				CountMinSketch[i][(e->Window + 1) % (_Period + 2)][j] = 0;
			}
		}		
		//LXD
		uint64_t HashValue = MurmurHash64B((const void*)e->ItemID, KEY_LEN, _HashSeed);
		unsigned long long hashvalue = HashValue;
		unsigned tmp = (((_SketchSize >> CMalpha)-1) << CMalpha);
		if(tmp == 0) tmp = 1;
		unsigned index = (hashvalue >> (_SketchArray * CMalpha)) % tmp;
		unsigned long long hashvalue_1 = hashvalue;
		unsigned index_1 = index;
		for (int i = 0; i < _SketchArray; i++) {

			unsigned addr = (index_1 + (hashvalue_1 & ((1 << CMalpha) - 1))) % (_SketchSize);

			CountMinSketch[i][e->Window % (_Period + 2)][addr]++;

			hashvalue_1 >>= CMalpha;

		}
		int EX = 0, EX2 = 0;
		bool IgnoreThisItem = false;
		for (int i = 2; i < _Period + 2; i++) {


			int ItemValue = INT_MAX;
			unsigned long long hashvalue_2 = hashvalue;
			unsigned index_2 = index;
			for (int j = 0; j < _SketchArray; j++) {

				unsigned addr = (index_2 + (hashvalue_2 & ((1 << CMalpha) - 1))) % _SketchSize;
				ItemValue = Min(ItemValue, CountMinSketch[j][(e->Window + i) % (_Period + 2)][addr]);

				if (ItemValue == 0)IgnoreThisItem = true;

				hashvalue_2 >>= CMalpha;

			}

			EX += ItemValue;

			EX2 += ItemValue * ItemValue;

		}
		if (IgnoreThisItem == true)return;

		if (EX2 * _Period - EX * EX < StableThreshold * _Period * _Period) {
			DX_cur = EX2 * _Period - EX * EX;

			unsigned HashBucket = HashValue % _NumOfHeavyHitterBuckets;

			int LeastSmoothIndex = 0, LeastSmoothTime = INT_MAX;	//If the bucket is full, we should kick the least smooth item with certain probability.

			for (int i = 0; i < ElementPerBucket; i++) {

				StableElement* e_ = &HeavyHitter[HashBucket][i];

				if (e_->ItemID == NULL) {	//Case 1: If it is an empty slot. Insert the coming item here.
					e_->StartStableTime = e->Window;
					e_->RecentStableTime = e->Window + 1;
					e_->ItemID = e->ItemID;
					return;
				}
				if (e_->RecentStableTime < e->Window) {		//Case 2: If the stable element is interrupted, we can safely replace it.


					if (e_->RecentStableTime - e_->StartStableTime > SmoothThreshold){

						stable_len = e_->RecentStableTime -e_->StartStableTime;

						ReportBuffer.push_back(make_pair(e_->ItemID, make_pair(e_->StartStableTime, e_->RecentStableTime)));// 	//If the item is stable for more than SmoothThreshold
					
					}
																												//times continuously, we will report this value.
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

			//LXD
			HeavyHitter[HashBucket][LeastSmoothIndex].RecentStableTime = e->Window + 1;		//No matter the original is kicked or not,
																							//we increase the RecentStableTime by 1.
			

			int Random = rand();

			if ((LeastSmoothTime + 1) * Random <= RAND_MAX)HeavyHitter[HashBucket][LeastSmoothIndex].ItemID = e->ItemID;
			//Kick the original item with probability 1/(fx+1).Where fx is 
			//the frequency of the least frequent element in the bucket.
			//1/(fx+1) can lead to an unbiased estimation by USS (SIGMOD2018).
		}

	}
	void Flush() {
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			for (int j = 0; j < ElementPerBucket; j++) {
				if (HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime > SmoothThreshold)
				{
					stable_len = HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime;//
					ReportBuffer.push_back(make_pair(HeavyHitter[i][j].ItemID,
						make_pair(HeavyHitter[i][j].StartStableTime, HeavyHitter[i][j].RecentStableTime)));//
				}
			}
		}
	}
	void ReportValue(double Throughput) {
		sort(ReportBuffer.begin(), ReportBuffer.end(), compare);
		int k = 0;
		int stablelength = 0;
		for (auto i = ReportBuffer.begin(); i != ReportBuffer.end(); i++) {
			ItemID_2 = changeID(i->first);
			TopKReport.insert(make_pair(ItemID_2,make_pair(i->second.second - i->second.first,
				make_pair(i->second.first, i->second.second))));//
			if (k >= TopK - 1 && stablelength != i->second.second - i->second.first)break;//
			k++, stablelength = i->second.second - i->second.first;//

		}
		printf("Comparison algorithm reports %u Top %d smooth items and the throughput is %lf Kibs.", (unsigned)TopKReport.size(), TopK, Throughput);

	}
	~CompareSketch() {
		for (int i = 0; i < _SketchArray; i++) {
			for (int j = 0; j < _Period + 2; j++) {
				delete[] CountMinSketch[i][j];
			}
		}
		for (int i = 0; i < _NumOfHeavyHitterBuckets; i++) {
			delete[] HeavyHitter[i];
		}
		delete HeavyHitter;
	}
};

#endif
