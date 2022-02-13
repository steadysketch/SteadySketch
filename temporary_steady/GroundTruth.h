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


class StandardAlgorithm {

private:

	int NumOfHashTable;

	unordered_map<string, int> ItemHashTable[_Period + 2] = {};

	map<string, pair<int, pair<int, int>>> StableItemHashTable = {};//<ItemID,<DX_sum,<start, end>>>
	//LXD
	unsigned TimeStamp;
	int DX_pre = 0;
	int DX_cur = 0;
	int stable_len = 0;
	string ItemID_1;


public:
	vector<pair<string, pair<int, pair<int, int>>>> ReportBuffer = {};		
	multimap<string, pair<int, pair<int, pair<int, int>>>> TopKReport = {};
	multimap<string, pair<int,int>> Instance_report = {};

	StandardAlgorithm() { NumOfHashTable = _Period + 2; }

	void Insert(Item* e) {

		//LXD----chenge ID format and clear all next time window
		string ItemID = {};
		int cal = 0;
		ItemID = changeID(e->ItemID);

		int LastTimeStamp = TimeStamp;
		TimeStamp = e->Window % NumOfHashTable;
		int NextStamp = (e->Window + 1) % NumOfHashTable;
		if(TimeStamp != LastTimeStamp){
			ItemHashTable[NextStamp].clear();
		}
		//LXD
		if(ItemHashTable[e->Window % NumOfHashTable][ItemID] == 0){
			cal = 1;
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
			//LXD-- DX calculating
			DX_cur = (EX2 * _Period - EX * EX);
			//LXD
			if(cal == 1){
				ItemID_1 = changeID(e->ItemID);
				Instance_report.insert(make_pair(ItemID_1,make_pair(e->Window,DX_cur)));
			}
		}

	}

};

#endif