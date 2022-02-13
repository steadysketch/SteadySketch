#include <thread>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <unordered_set>
#include "LRU.h"
#include "utils.h"
#include "our_LRU.h"
using namespace std;
void work(const char *data_name, FILE *result_file, int window_size = 20000, int window_num = 400, int _cache_size = 0x200000, double delta = 0.8)
{
    timeval stime, ttime;
    gettimeofday(&stime, NULL);

    int NumOfPackage = 0;
    Item ItemVector[33000000] = {};

    ifstream Data;
    Data.open(data_name, ios::binary);
    if (!Data.is_open())
        return;
    int time = 0;
    do
    {
        double AccurateTime = 0;
        Data.read(ItemVector[NumOfPackage].ItemID, 13);
        if (NumOfPackage < (time * window_size))
            ItemVector[NumOfPackage].Window = time;
        else
        {
            ++time;
            ItemVector[NumOfPackage].Window = time;
        }
        ++NumOfPackage;
    } while (ItemVector[NumOfPackage - 1].Window != window_num);
    Data.close();

    gettimeofday(&ttime, NULL);
    fprintf(stderr, "read use %ldus\n", (ttime.tv_sec - stime.tv_sec) * 1000000 + (ttime.tv_usec - stime.tv_usec));

    gettimeofday(&stime, NULL);

    LRU *l = new LRU(_cache_size / 4);
    our_LRU *ol = new our_LRU(_cache_size, delta);
    int i;
    for (i = 0; i < NumOfPackage; ++i)
        if (ItemVector[i].Window < window_num / 4)
        {
            l->access(ItemVector[i]);
            ol->access(ItemVector[i]);
        }
        else
            break;
    l->clear_access();
    ol->clear_access();
    for (; i < NumOfPackage; ++i)
    {
        l->access(ItemVector[i]);
        ol->access(ItemVector[i]);
    }

    gettimeofday(&ttime, NULL);
    fprintf(stderr, "access use %ldus\n", (ttime.tv_sec - stime.tv_sec) * 1000000 + (ttime.tv_usec - stime.tv_usec));

    fprintf(stderr, "%d,%d,%d,%lf,%lf,%lf\n", window_size, window_num, _cache_size, delta, l->get_hit_rate(), ol->get_hit_rate());

    fprintf(result_file, "%d,%d,%d,%lf,%lf,%lf\n", window_size, window_num, _cache_size, delta, l->get_hit_rate(), ol->get_hit_rate());

    // printf(",%d,%d,%d,%d\n", ol->cnt00, ol->cnt01, ol->cnt10, ol->cnt11);

    delete l;
    delete ol;
}

void run1(const char *data_name, const char *result_name)
{
    FILE *result_file = fopen(result_name, "w");
    for (int _window_size = 32000; _window_size <= 32000; _window_size *= 2)
    {
        int _window_num = 32000000 / _window_size;
        for (int _total_size = 20000; _total_size <= 130000; _total_size += 10000)
        {
            for (double delta = 0.5; delta <= 0.9; delta += 0.2)
                work(data_name, result_file, _window_size, _window_num, _total_size, delta);
        }
    }
    fclose(result_file);
}

void run2(const char *data_name, const char *result_name)
{
    FILE *result_file = fopen(result_name, "w");
    for (int _window_size = 8000; _window_size <= 8000; _window_size += 1000)
    {
        int _window_num = 8000000 / _window_size;
        for (int _total_size = 5000; _total_size <= 16000; _total_size += 1000)
            for (double delta = 0.3; delta <= 0.7; delta += 0.2)
                work(data_name, result_file, _window_size, _window_num, _total_size, delta);
    }
    fclose(result_file);
}

int main()
{
    thread *threads[11];
    char result_name[11][20];
    char data_name[11][20];
    /*
    for (int i = 1; i <= 5; ++i)
    {
        sprintf(result_name[i], "result_%d.csv", i);
        sprintf(data_name[i], "data/%d.dat", i);
        threads[i - 1] = new thread(run1, data_name[i], result_name[i]);
    }
    for (int i = 0; i < 5; ++i)
        threads[i]->join();
    for (int i = 6; i <= 10; ++i)
    {
        sprintf(result_name[i], "result_%d.csv", i);
        sprintf(data_name[i], "data/%d.dat", i);
        threads[i - 1] = new thread(run1, data_name[i], result_name[i]);
    }
    */
    threads[10] = new thread(run2, "campus.dat", "result.csv");

    for (int i = 10; i < 11; ++i)
        threads[i]->join();

    // work(8000, 1000, 4096, 0.8);
    return 0;
}