#ifndef Strawman_H
#define Strawman_H

#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include <vector>
#include <map>
#include <unordered_map>

using namespace std;

// string changeID(char* ItemID){

// 	string change_id = {};
// 	for (int i = 0; i < 15; i++){
// 		char p = ItemID[i];
// 		unsigned int k = (unsigned int) p;
// 		change_id += to_string(k);
// 	}
// 	return change_id;
// }

class CompareSketch {

private:

	int* CountMinSketch[_SketchArray][_Period + 2] = {};
	//LXD
	unsigned TimeStamp;
	string ItemID_1;
	int DX_cur = 0;
	int stable_len = 0;
	const unsigned CMalpha = 6;
    int _SketchSize = 0;


public:

	vector<pair<char*, pair<int, int>>> ReportBuffer = {};		
	multimap<string, pair<int, pair<int, pair<int, int>>>> TopKReport = {};
	multimap<string, pair<int,int>> Instance_report = {};

	CompareSketch(int SketchSize) {
        _SketchSize = SketchSize;

		for (int i = 0; i < _SketchArray; i++) {
			for (int j = 0; j < _Period + 2; j++) {
				CountMinSketch[i][j] = new int[_SketchSize]();
			}
		}
	}

	void Insert(Item* e,int KEY_LEN, int _HashSeed) {

		//LXD clear all next time window buckets
		int LastTimeStamp = TimeStamp;
		TimeStamp = e->Window % (_Period + 2);

		if(LastTimeStamp != TimeStamp){
			for(int i = 0; i < _SketchArray; i++){
				for (int j = 0; j < _SketchSize; j ++)
				CountMinSketch[i][(e->Window + 1) % (_Period + 2)][j] = 0;
			}
		}

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
			ReportBuffer.push_back(make_pair(e->ItemID,make_pair(e->Window,DX_cur)));
			
		}

	}
	void Duplicate(){
		cout<<"before duplicate remove "<<ReportBuffer.size()<<endl;
		int find = 0;

		for (auto i= ReportBuffer.begin(); i != ReportBuffer.end();i++){
			ItemID_1 = changeID(i->first);
			auto count = Instance_report.count(ItemID_1);
			auto it = Instance_report.find(ItemID_1);
			int find = 0;
			while(count){
				if(it->second.first == i->second.first){
					find = 1;
				}
				count--;
				it++;
			}
			if(find == 0){
				Instance_report.insert(make_pair(ItemID_1,make_pair(i->second.first, i->second.second)));
			}
		}

		cout<<"after duplicate remove : "<<Instance_report.size()<<endl;

	}
	
};

#endif