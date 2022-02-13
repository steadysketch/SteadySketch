#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
using namespace std;

bool compare(const pair<string, pair<int, pair<int, int>>> &t1, const pair<string, pair<int, pair<int, int>>> &t2)
{
    return t1.second.second.second - t1.second.second.first > t2.second.second.second - t2.second.second.first;
}

struct Item
{ //An Item is a unit in data set. It contains the item's ID and its coming window.

    char ItemID[15] = {};

    int Window = 0;
};

struct StableElement
{ //Like Unbiased Space Saving. We use the StableElement to save the current stable item.

    int StartStableTime = 0; //To save the first time the item is stable.

    int RecentStableTime = 0; //To save the last time the item is stable.

    char *ItemID = NULL; //To save the item itself.

    //LXD
    int DX_sum;
};

//LXD
string changeID(const char *ItemID)
{

    string change_id = {};
    for (int i = 0; i < 15; i++)
    {
        //char str[100];
        char p = ItemID[i];
        unsigned int k = (unsigned int)p;
        //itoa(k,str,16);
        change_id += to_string(k);
    }
    return change_id;
}
#endif