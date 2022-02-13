#ifndef parm_H
#define parm_H
#include <bits/stdc++.h>
#include "MurmurHash.h"

#define Min(a,b) 	((a) < (b) ? (a) : (b))

#define Max(a,b) 	((a) > (b) ? (a) : (b))

#define INT_MAX				((1<<30)-1)

//#define BloomFilterSize		(0x40000)	//In bits.

#define StableThreshold		(0x05)	//If an item X satisfies DX < StableThreshold, we say the item is stable.

#define SmoothThreshold		(0x01)  //If an item X appears is recognized as stable more than SmoothThreshold times 
//continuously, we say the item is smooth.

#define ElementPerBucket	(0x04)	//In general, we have 4 elements per bucket. That is, we can hold at most 4 items
									//if they maps to the same bucket in HeavyHitter.


struct Item {	//An Item is a unit in data set. It contains the item's ID and its coming window.

	char ItemID[15] = {};

	int Window = 0;
};

struct StableElement {		//Like Unbiased Space Saving. We use the StableElement to save the current stable item.

	int StartStableTime = 0;	//To save the first time the item is stable.

	int RecentStableTime = 0;	//To save the last time the item is stable.

	char* ItemID = NULL;	//To save the item itself.

	//LXD
	int DX_sum;
};



const int _SketchArray = 0x2; // the number of array in RollingSKetch

const int _Period = 0x05;// We define an item is a steady item must be uninterrupted in previous _period time winddows

const int  _FilterArray = 0x03; // the number of array in SteadyFilter

int   _HashSeed = 0x100;// hashseed in murmurhash

const float prop = 0.2; // the proportion of steadyfilter space in all memory size 

const int memory_size = 100; // the memory occupied by steadyfilter and rollingsketch







#endif