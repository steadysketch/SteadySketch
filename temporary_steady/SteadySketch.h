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
	const unsigned w = _Period + 2, longdivshort = (int)sizeof(long long) / sizeof(short), RBFalpha = 5, GSalpha = 5;
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
	vector<pair<char*, pair<int, int>>> ReportBuffer = {};		
	multimap<string, pair<int, pair<int, pair<int, int>>>> TopKReport = {};
	multimap<string, pair<int,int>> Instance_report = {};

	SteadySketch(int _d, int _L, int _RBFL, int _RBFNum , int _hash_seed = 100) {
		d = _d;
		L = _L;
		RBFNum = _RBFNum;
		RBFL = _RBFL;
		hash_seed = _hash_seed;
		RBF = new short[RBFNum*RBFL]();
		GS = new unsigned char[d * L * w]();
		TimeStamp = 0;

		RBF_test = new short[RBFNum * 1398101];
		
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

			ReportBuffer.push_back(make_pair(e->ItemID,make_pair(e->Window,DX_cur)));		

			
		}



	}
	void flush(){
		for (auto i= ReportBuffer.begin(); i != ReportBuffer.end();i++){
			ItemID_1 = changeID(i->first);
			Instance_report.insert(make_pair(ItemID_1,make_pair(i->second.first, i->second.second)));
		}
	}
};

#endif