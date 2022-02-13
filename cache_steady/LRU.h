#ifndef _LRU_H_
#define _LRU_H_

#include <string>
#include <unordered_map>
#include "utils.h"
#include "list.h"
#include "MurmurHash.h"
using namespace std;

class LRU
{
protected:
    unordered_map<string, list_node *> d;
    list *cache;
    int cache_limit;
    int success_access;
    int failed_access;

public:
    LRU(const int &_cache_limit)
    {
        cache_limit = _cache_limit;
        cache = new list(cache_limit);
        success_access = 0;
        failed_access = 0;
    }
    void access(const Item &x)
    {
        string s = changeID(x.ItemID);
        if (d.find(s) != d.end())
        {
            ++success_access;
            cache->adjust(d[s]);
            return;
        }
        ++failed_access;
        if (cache->is_full())
        {
            string *tmp = cache->get_first_item();
            d.erase(*tmp);
            cache->del_first_item();
        }
        d[s] = cache->insert(s);
    }
    int get_success_access() const { return success_access; }
    int get_failed_access() const { return failed_access; }
    double get_hit_rate() const { return (double)success_access / (success_access + failed_access); }
    void clear_access() { success_access = failed_access = 0; }
    virtual ~LRU() { delete cache; }
};
/*
class LRU
{
protected:
    list **cache;
    int success_access;
    int failed_access;
    int cache_size;
    int cache_line_size;
    int way_number;
    int set_number;

public:
    LRU(int _cache_size, int _cache_line_size, int _way_number) : cache_size(_cache_size), cache_line_size(_cache_line_size), way_number(_way_number)
    {
        set_number = cache_size / cache_line_size / way_number;
        cache = new list *[set_number];
        for (int i = 0; i < set_number; ++i)
            cache[i] = new list(way_number);
        success_access = failed_access = 0;
    }
    void access(const Item &x)
    {
        string s = changeID(x.ItemID);
        uint64_t HashValue = MurmurHash64B((const void *)x.ItemID, 15, 100);
        int set_id = HashValue % set_number;
        list_node *it = cache[set_id]->find(s);
        if (it != NULL)
        {
            ++success_access;
            cache[set_id]->adjust(it);
            return;
        }
        ++failed_access;
        if (cache[set_id]->is_full())
        {
            string *tmp = cache[set_id]->get_first_item();
            cache[set_id]->del_first_item();
        }
        cache[set_id]->insert(s);
    }
    int get_success_access() const { return success_access; }
    int get_failed_access() const { return failed_access; }
    double get_hit_rate() const { return (double)success_access / (success_access + failed_access); }
    void clear_access() { success_access = failed_access = 0; }
    virtual ~LRU()
    {
        for (int i = 0; i < set_number; ++i)
            delete cache[i];
        delete cache;
    }
};
*/
#endif