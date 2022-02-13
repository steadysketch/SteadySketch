#ifndef _STABLE_ITEM_SKETCH_H_
#define _STABLE_ITEM_SKETCH_H_

#include <iostream>
#include "MurmurHash.h"
#include <random>
#include <mmintrin.h>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "utils.h"

#define Min(a, b) ((a) < (b) ? (a) : (b))

#define Max(a, b) ((a) > (b) ? (a) : (b))

#define INT_MAX ((1 << 30) - 1)

#define StableThreshold (0x05) // If an item X satisfies DX < StableThreshold, we say the item is stable.

#define SmoothThreshold (0x01) // If an item X appears is recognized as stable more than SmoothThreshold times
// continuously, we say the item is smooth.

#define ElementPerBucket (0x04) //In general, we have 4 elements per bucket. That is, we can hold at most 4 items \
								//if they maps to the same bucket in HeavyHitter.

#define KEY_LEN (0x0d) // Key length. In our data set, the key length is 13 bytes. We use it to calculate hash values.

const int _NumOfArray = 0x02, _Period = 0x05, _NumOfBloomFilterBits = 0x03, _HashSeed = 0x100;
const unsigned longdivshort = (int)sizeof(long long) / sizeof(short); // 4

using namespace std;

class GroupSketch
{
private:
    short *RBF;
    unsigned char *GS;
    unsigned d;
    unsigned L;
    unsigned RBFNum;
    unsigned RBFL;
    unsigned hash_seed;
    const unsigned w = _Period + 2;
    unsigned RBFalpha = 10;
    unsigned GSalpha = 10;
    const uint64_t RBFmod = 0x0001000100010001;
    unsigned TimeStamp;
    int TopK;
    int NumOfHeavyHitterBuckets;

public:
    StableElement **HeavyHitter;
    vector<pair<string, pair<int, pair<int, int>>>> ReportBuffer = {};
    // To report the smooth item. ReportBuffer contains the smooth items.
    multimap<string, pair<int, pair<int, pair<int, int>>>> TopKReport = {};
    // GroupSketch(int _d, int _L, int _RBFNum = 3, int _RBFL = BloomFilterSize, int _hash_seed = 100) {
    GroupSketch(int _d, int _L, int _RBFL, int _TopK = 0x1000000, int _NumOfHeavyHitterBuckets = 0x1000, int _RBFNum = 3, int _hash_seed = 100)
    {
        d = _d;
        L = _L;
        RBFNum = _RBFNum;
        RBFL = _RBFL;
        TopK = _TopK;
        NumOfHeavyHitterBuckets = _NumOfHeavyHitterBuckets;
        hash_seed = _hash_seed;
        RBFalpha = log(RBFL) / log(2);
        GSalpha = log(L) / log(2);
        RBF = new short[RBFNum * RBFL]();
        GS = new unsigned char[d * L * w]();
        TimeStamp = 0;
        HeavyHitter = new StableElement *[_NumOfHeavyHitterBuckets];
        for (int i = 0; i < _NumOfHeavyHitterBuckets; i++)
        {
            HeavyHitter[i] = new StableElement[ElementPerBucket]();
        }
    }
    bool Insert(Item *e)
    {
        int LastTimeStamp = TimeStamp;
        TimeStamp = e->Window % w;
        int NextStamp = TimeStamp + 1;
        if (NextStamp == w)
            NextStamp = 0;

        if (TimeStamp != LastTimeStamp)
        {
            long long *p = (long long *)RBF;
            int step = RBFNum * RBFL / longdivshort;
            for (int i = 0; i < step; i++)
                p[i] &= (~(RBFmod << NextStamp));

            // LXD-- clear all next time window buckets
            unsigned GS_len = d * L * w;
            int step_gs = NextStamp;
            while (step_gs < GS_len)
            {
                GS[step_gs] = 0;
                step_gs += w;
            }
        }

        uint64_t HashValue = MurmurHash64B((const void *)e->ItemID, KEY_LEN, hash_seed);

        int TagRBF = 0;
        unsigned long long hashvalue = HashValue;
        unsigned tmp = RBFL - (1 << RBFalpha);
        if (tmp == 0)
            tmp = 1;
        unsigned index = (hashvalue >> (RBFNum * RBFalpha)) % tmp;
        for (int i = 0; i < RBFNum; i++)
        {
            unsigned addr = index + (hashvalue & ((1 << RBFalpha) - 1));
            TagRBF += (RBF[addr] & (1 << TimeStamp)) >> TimeStamp;
            RBF[addr] |= 1 << TimeStamp;
            if (((~(RBF[addr] | (1 << NextStamp))) & ((1 << w) - 1)) != 0)
                TagRBF = RBFNum;
            index += RBFL;
            hashvalue >>= RBFalpha;
        }
        hashvalue = HashValue;
        index = (hashvalue >> (d * GSalpha)) % (L - (1 << GSalpha));
        unsigned char Group[2][sizeof(short) << 3] = {};
        for (int i = 0; i < w; i++)
            Group[0][i] = Group[1][i] = 0xff;

        for (int i = 0; i < d; i++)
        {
            unsigned addr = (index + (hashvalue & ((1 << GSalpha) - 1))) * w;
            GS[addr + TimeStamp]++;
            GS[addr + NextStamp] = 0;

            index += L;
            hashvalue >>= GSalpha;
            if (TagRBF >= RBFNum)
                continue;
            for (int j = 0; j < w; j++)
            {
                Group[0][j] = Min(Group[0][j], GS[addr + j]);
                Group[1][j] = Min(Group[1][j], GS[addr + j] + (unsigned char)0x80);
            }
            Group[0][TimeStamp] = Group[1][TimeStamp] = 0;
        }
        if (TagRBF >= RBFNum)
            return false;

        int EX[2] = {}, EX2[2] = {};

        for (int i = 0; i < w; i++)
        {
            EX[0] += (unsigned)Group[0][i];
            EX2[0] += ((unsigned)Group[0][i]) * Group[0][i];

            EX[1] += (unsigned)Group[1][i];
            EX2[1] += ((unsigned)Group[1][i]) * Group[1][i];
        }

        if (EX2[0] * (w - 2) - EX[0] * EX[0] < StableThreshold * (w - 2) * (w - 2) ||
            EX2[1] * (w - 2) - EX[1] * EX[1] < StableThreshold * (w - 2) * (w - 2))
        { // If DX>StableThreshold. We say this item is stable.
            return true;
        }
        else
            return false;
    }

    bool find(Item *e)
    {
        uint64_t HashValue = MurmurHash64B((const void *)e->ItemID, KEY_LEN, hash_seed);
        unsigned HashBucket = HashValue % NumOfHeavyHitterBuckets;
        for (int i = 0; i < ElementPerBucket; i++)
        {
            StableElement *e_ = &HeavyHitter[HashBucket][i];
            if (e_->ItemID != NULL && memcmp(e_->ItemID, e->ItemID, KEY_LEN) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void Flush()
    {
        for (int i = 0; i < NumOfHeavyHitterBuckets; i++)
        {
            for (int j = 0; j < ElementPerBucket; j++)
            {
                if (HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime > SmoothThreshold)
                {
                    // LXD -- chenge ItemID format && change the DX_sum to DX_ave
                    string ItemID_2 = changeID(HeavyHitter[i][j].ItemID);
                    int stable_len = HeavyHitter[i][j].RecentStableTime - HeavyHitter[i][j].StartStableTime;
                    // HeavyHitter[i][j].DX_sum = HeavyHitter[i][j].DX_sum/stable_len;
                    ReportBuffer.push_back(make_pair(ItemID_2, make_pair(HeavyHitter[i][j].DX_sum, make_pair(HeavyHitter[i][j].StartStableTime, HeavyHitter[i][j].RecentStableTime))));
                }
            }
        }
    }

    void ReportValue()
    { // Report the smooth items.
        sort(ReportBuffer.begin(), ReportBuffer.end(), compare);
        int k = 0;
        int stablelength = 0;
        for (auto i = ReportBuffer.begin(); i != ReportBuffer.end(); i++)
        {

            TopKReport.insert(make_pair(i->first, make_pair(i->second.first, make_pair(i->second.second.second - i->second.second.first, make_pair(i->second.second.first, i->second.second.second))))); //
            if (k >= TopK - 1 && stablelength != i->second.second.second - i->second.second.first)
                break;                                                            //
            k++, stablelength = i->second.second.second - i->second.second.first; //
        }
    }
    ~GroupSketch()
    {
        for (int i = 0; i < NumOfHeavyHitterBuckets; i++)
        {
            delete[] HeavyHitter[i];
        }
        delete[] GS;
        delete HeavyHitter;
    }
};

#endif