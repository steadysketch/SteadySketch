#ifndef _OUR_LRU_H_
#define _OUR_LRU_H_

#include <unordered_set>
#include "LRU.h"
#include "utils.h"
#include "StableItemSketch.h"
using namespace std;

class our_LRU : public LRU
{
protected:
    GroupSketch *g;
    unordered_map<string, list_node *> d1;
    list *cache1;
    double delta;
    int cache1_limit;
    int total_size;
    int RBF_size;
    int GS_size;
    int heavyhitter_size;
    int topk_size;

public:
    int cnt00, cnt01, cnt10, cnt11;
    our_LRU(int _total_size, double _delta) : LRU(int(_total_size * _delta) / 4)
    {
        total_size = _total_size;
        delta = _delta;

        int tmp = total_size - cache_limit * 4;
        RBF_size = 50000;
        GS_size = 50000;
        heavyhitter_size = 8;
        topk_size = 0;
        g = new GroupSketch(3, GS_size / 21, RBF_size / 6, 0, heavyhitter_size / 8);
        cnt00 = cnt01 = cnt10 = cnt11 = 0;
        cache1_limit = tmp / 4;
        cache1 = new list(cache1_limit);
    }

    void access(Item &x)
    {
        string s = changeID(x.ItemID);
        if (d1.find(s) != d1.end())
        {
            ++success_access;
            if (g->Insert(&x))
            {
                cache1->adjust(d1[s]);
            }
        }
        else if (d.find(s) != d.end())
        {
            ++success_access;
            if (g->Insert(&x))
            {
                cache->erase(d[s]);
                d.erase(s);

                if (cache1->is_full())
                {
                    string tmp = *cache1->get_first_item();
                    d1.erase(tmp);
                    cache1->del_first_item();
                    if (cache->is_full())
                    {
                        string *tmp1 = cache->get_first_item();
                        d.erase(*tmp1);
                        cache->del_first_item();
                    }
                    d[tmp] = cache->insert(tmp);
                }
                d1[s] = cache1->insert(s);
            }
            else
            {
                cache->adjust(d[s]);
            }
        }
        else
        {
            ++failed_access;

            if (g->Insert(&x))
            {
                if (cache1->is_full())
                {
                    string *tmp = cache1->get_first_item();
                    d1.erase(*tmp);
                    cache1->del_first_item();
                }
                d1[s] = cache1->insert(s);
            }
            else
            {
                if (cache->is_full())
                {
                    string *tmp = cache->get_first_item();
                    d.erase(*tmp);
                    cache->del_first_item();
                }
                d[s] = cache->insert(s);
            }
        }
    }
    ~our_LRU()
    {
        delete g;
        delete cache1;
    }
};
/*
class our_LRU : public LRU
{
protected:
    GroupSketch *g;
    list **cache_1;
    int cache_size_1;
    int way_number_1;
    int set_number_1;

public:
    our_LRU(int _cache_size, int _cache_line_size, int _way_number, double delta) : LRU(_cache_size * delta, _cache_line_size, _way_number * delta)
    {
        g = new GroupSketch(3, 75000 / 21, 25000 / 6, 0, 1);
        cache_size_1 = _cache_size * (1 - delta);
        way_number_1 = _way_number * (1 - delta);
        set_number_1 = cache_size_1 / cache_line_size / way_number_1;
        cache_1 = new list *[set_number_1];
        for (int i = 0; i < set_number_1; ++i)
            cache_1[i] = new list(way_number_1);
    }
    void access(Item &x)
    {
        string s = changeID(x.ItemID);
        uint64_t HashValue = MurmurHash64B((const void *)x.ItemID, 15, 100);
        int set_id = HashValue % set_number;
        int set_id_1 = HashValue % set_number_1;
        list_node *it = cache[set_id]->find(s);
        list_node *it_1 = cache_1[set_id_1]->find(s);
        if (it_1 != NULL)
        {
            ++success_access;
            if (g->Insert(&x))
            {
                cache_1[set_id_1]->adjust(it_1);
            }
        }
        else if (it != NULL)
        {
            ++success_access;
            if (g->Insert(&x))
            {
                cache[set_id]->erase(it);

                if (cache_1[set_id_1]->is_full())
                {
                    string tmp = *cache_1[set_id_1]->get_first_item();
                    cache_1[set_id_1]->del_first_item();
                    if (cache[set_id]->is_full())
                    {
                        string *tmp1 = cache[set_id]->get_first_item();
                        cache[set_id]->del_first_item();
                    }
                    cache[set_id]->insert(tmp);
                }
                cache_1[set_id_1]->insert(s);
            }
            else
            {
                cache[set_id]->adjust(it);
            }
        }
        else
        {
            ++failed_access;

            if (g->Insert(&x))
            {
                if (cache_1[set_id_1]->is_full())
                {
                    string *tmp = cache_1[set_id_1]->get_first_item();
                    cache_1[set_id_1]->del_first_item();
                }
                cache_1[set_id_1]->insert(s);
            }
            else
            {
                if (cache[set_id]->is_full())
                {
                    string *tmp = cache[set_id]->get_first_item();
                    cache[set_id]->del_first_item();
                }
                cache[set_id]->insert(s);
            }
        }
    }
    ~our_LRU()
    {
        delete g;
        for (int i = 0; i < set_number_1; ++i)
            delete cache_1[i];
        delete cache_1;
    }
};
*/
#endif