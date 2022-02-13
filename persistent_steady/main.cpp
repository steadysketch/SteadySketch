#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include "GroundTruth.h"
#include "strawman.h"
#include "SteadySketch.h"
#define BeginTime   (0x5AAA6E50)
using namespace std;

Item ItemVector[0x4000000] = {};
int NumofPacket = 0;
int KEY_LEN = 0xd;

int main(){

    //data file open
    ifstream Data;
    Data.open("130000.dat", ios::binary);//open dataset file --- caida path
    if(!Data.is_open()) return -1;

    //Read dataset file 
    int ts_window = 0;
    do{
        double AccurateTime = 0;
        Data.read(ItemVector[NumofPacket].ItemID, KEY_LEN);
        Data.read((char*)&AccurateTime, sizeof(double));
        ts_window = NumofPacket/10000;
        ItemVector[NumofPacket].Window = ts_window;
        NumofPacket ++;
    }while (!Data.eof());



    /**********  G R O U N D  T R U T H      **********/
    StandardAlgorithm  GroundTruth = {};
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < NumofPacket; i ++) GroundTruth.Insert(&ItemVector[i]);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli>tm = end - start;
    GroundTruth.Flush();
    GroundTruth.ReportValue(NumofPacket/(1.0 * tm.count()));
    
    cout<<NumofPacket<<" "<<GroundTruth.TopKReport.size()<<endl;


    /********** S T E A D Y   S K E T C H  ********************/
    {
        int _FilterSize = prop * memory_size * 1024 / _FilterArray;
        int _SketchSize = (_FilterSize * _FilterArray)/((_Period + 2) * _SketchArray * (prop/(1-prop)));

        SteadySketch Steady(_SketchArray, _SketchSize, _FilterSize,_FilterArray);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NumofPacket; i++) Steady.Insert(&ItemVector[i],KEY_LEN);
        Steady.Flush();
        auto  end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli>tm = end - start;
                
        Steady.ReportValue(NumofPacket / (1.0 * tm.count()));
    
        {   //Evaluate
            int SUMRANGE = 0;
            for (auto i = Steady.TopKReport.begin(); i != Steady.TopKReport.end(); i++) {
                auto Count = GroundTruth.TopKReport.count(i->first);
                auto it = GroundTruth.TopKReport.find(i->first);
                int Find = 0;
                while (Count) {
                    if (it->second == i->second){//
                        Find = 1;
                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;

            }
            printf(" %d are right.\n", SUMRANGE);
            cout<< "RR :"<<SUMRANGE * 1.0 / GroundTruth.TopKReport.size()<<"PR is:"<<SUMRANGE * 1.0 / Steady.TopKReport.size()<<endl;

           
        }
                

    }



    /**********  S T R A W M A N   S O L U T I O N      **********/
    {
        int _SketchSize = (1024 * memory_size)/(sizeof(int) * _SketchArray * (_Period + 2));
        CompareSketch Strawman(_SketchSize);
        //start_t = clock();
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NumofPacket; i++) Strawman.Insert(&ItemVector[i],KEY_LEN);
        //end_t = clock();
        Strawman.Flush();
        auto  end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli>tm = end - start;                    
        Strawman.ReportValue(NumofPacket / (1.0 * tm.count()));
    
        {   //Evaluate
            int SUMRANGE = 0;
            int  sameid_count_CM = 0;
            int  sameid_count_Ground = 0;
            for (auto i = Strawman.TopKReport.begin(); i != Strawman.TopKReport.end(); i++) {
                auto Count = GroundTruth.TopKReport.count(i->first);
                auto it = GroundTruth.TopKReport.find(i->first);
                auto count_CM = Strawman.TopKReport.count(i->first);
                int Find = 0;
                sameid_count_Ground += Count;
                sameid_count_CM += count_CM;
                while (Count) {
                    if (it->second == i->second){
                        Find = 1;      
                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;

            }

            printf(" %d are right.\n", SUMRANGE);
            cout<<"Strawman RR: "<<SUMRANGE * 1.0 / GroundTruth.TopKReport.size()<<"Strawman PR: "<<SUMRANGE * 1.0 / Strawman.TopKReport.size()<<endl;

          

        }
                    
    
    
    
    
    }
    




}