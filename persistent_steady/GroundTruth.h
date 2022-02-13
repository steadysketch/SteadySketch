#ifndef StandardAlgorithm_H
#define StandardAlgorithm_H

#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include <vector>
#include <map>
#include <unordered_map>

using namespace std;

string changeID(char* ItemID){

	string change_id = {};
	for (int i = 0; i < 15; i++){
		char p = ItemID[i];
		unsigned int k = (unsigned int) p;
		change_id += to_string(k);
	}
	return change_id;
}

bool compare(const pair<string, pair<int, int>>& t1, const pair<string, pair<int, int>>& t2) {
	return t1.second.second - t1.second.first > t2.second.second - t2.second.first;
}

class StandardAlgorithm {

private:

	int NumOfHashTable;

	unordered_map<string, int> ItemHashTable[_Period + 2] = {};

	map<string, pair<int, int>> StableItemHashTable = {};//<ItemID,<DX_sum,<start, end>>>
	//LXD
	unsigned TimeStamp;
	int DX_pre = 0;
	int DX_cur = 0;
	int stable_len = 0;


public:
	vector<pair<string, pair<int, int>>> ReportBuffer = {};		
	//To report the smooth item. ReportBuffer contains the smooth items.
	//<ItemID,<DX,<start, end>>>

	multimap<string,pair<int, pair<int, int>>> TopKReport = {};
	//<ItemID, <DX, <stable_len, <start, end>>>>

	StandardAlgorithm() { NumOfHashTable = _Period + 2; }

	void Insert(Item* e) {

		//LXD----chenge ID format and clear all next time window
		string ItemID = {};
		ItemID = changeID(e->ItemID);

		int LastTimeStamp = TimeStamp;
		TimeStamp = e->Window % NumOfHashTable;
		int NextStamp = (e->Window + 1) % NumOfHashTable;
		if(TimeStamp != LastTimeStamp){
			ItemHashTable[NextStamp].clear();
		}

		ItemHashTable[e->Window % NumOfHashTable][ItemID]++;

		int EX = 0, EX2 = 0;

		bool IgnoreThisItem = false;

		for (int i = 2; i < NumOfHashTable; i++) {

			int ItemValue = ItemHashTable[(e->Window + i) % NumOfHashTable][ItemID];

			if (ItemValue == 0)IgnoreThisItem = true;

			EX += ItemValue;

			EX2 += ItemValue * ItemValue;

		}
		if (IgnoreThisItem == true)return;

		if (EX2 * _Period - EX * EX < StableThreshold * _Period * _Period) {

			if (StableItemHashTable.find(ItemID) == StableItemHashTable.end()) {//first come

				StableItemHashTable[ItemID].first = e->Window;//

				StableItemHashTable[ItemID].second = e->Window + 1;//

			}
			else if (StableItemHashTable[ItemID].second < e->Window) {// //interruptted

				if (StableItemHashTable[ItemID].second - StableItemHashTable[ItemID].first > SmoothThreshold) {//
					stable_len = StableItemHashTable[ItemID].second - StableItemHashTable[ItemID].first;//插入到reportbuffer中时 heavyhitter中数值会变化
					ReportBuffer.push_back(make_pair(ItemID, StableItemHashTable[ItemID]));
				}

				StableItemHashTable[ItemID].first = e->Window;//

				StableItemHashTable[ItemID].second = e->Window + 1;//
				
				
			}
			// increment 
			else {
				StableItemHashTable[ItemID].second = e->Window + 1;//
				
			}
		}

	}
	void Flush() {
		for (auto i = StableItemHashTable.begin(); i != StableItemHashTable.end(); i++)
			if ((i->second.second) - (i->second.first) > SmoothThreshold){//
				stable_len = i->second.second - i->second.first;//
				//i->second.first = i->second.first/(stable_len);//
				ReportBuffer.push_back(make_pair(i->first, i->second));
			}

	}
	void ReportValue(double Throughput) {
		sort(ReportBuffer.begin(), ReportBuffer.end(), compare);
		int k = 0;
		int stablelength = 0;
		for (auto i = ReportBuffer.begin(); i != ReportBuffer.end(); i++) {

			TopKReport.insert(make_pair(i->first, make_pair(i->second.second - i->second.first,
				make_pair(i->second.first, i->second.second))));//
			if (k >= TopK - 1 && stablelength != i->second.second - i->second.first)break;//
			k++, stablelength = i->second.second - i->second.first;//
		}
		printf("Standard Algorithm reports %u smooth items (%u Top %d smooth items) and the throughput is %lf Kibs.\n",
			 (unsigned)ReportBuffer.size(), (unsigned)TopKReport.size(), TopK, Throughput);

	}
};
#endif