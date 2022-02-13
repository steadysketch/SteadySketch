#include <bits/stdc++.h>
#include "MurmurHash.h"
#include "parm.h"
#include "GroundTruth.h"
#include "strawman.h"
#include "SteadySketch.h"

using namespace std;

Item ItemVector[0x4000000] = {};
int NumofPacket = 0;
int KEY_LEN = 0xd;

int main(){

    //data file open
    ifstream Data;
    Data.open("campus.dat", ios::binary);//open dataset file --- caida 
    if(!Data.is_open()) return -1;

    //Read dataset file 
    int ts_window = 0;
    do{
        Data.read(ItemVector[NumofPacket].ItemID, KEY_LEN);
        ts_window = NumofPacket/10000;
        ItemVector[NumofPacket].Window = ts_window;
        NumofPacket ++;
    }while (!Data.eof());
    // while (ItemVector[NumofPacket].Window<10);
    

    /**********  G R O U N D  T R U T H      **********/
    StandardAlgorithm  GroundTruth = {};
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < NumofPacket; i ++) GroundTruth.Insert(&ItemVector[i]);
    auto end = std::chrono::high_resolution_clock::now();
    
    cout<<NumofPacket<<" "<<GroundTruth.Instance_report.size()<<endl;

    /**********  S T E A D Y   S K E T C H   **********/
    {
        int _FilterSize = prop *  memory_size * 1024/_FilterArray;
        int _SketchSize = (_FilterSize * _FilterArray)/((_Period + 2) * _SketchArray * (prop/(1 - prop)));
        SteadySketch SteadySketch(_SketchArray,_SketchSize,_FilterSize,_FilterArray);
        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < NumofPacket; i++) SteadySketch.Insert(&ItemVector[i],KEY_LEN);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        SteadySketch.flush();
        cout<<SteadySketch.Instance_report.size()<<endl;

        {   //Evaluate
            int SUMRANGE = 0;
            float DX_sum = 0;
            float DX_error_AVE = 0;
 
            for (auto i = SteadySketch.Instance_report.begin(); i != SteadySketch.Instance_report.end(); i++) {
                auto Count = GroundTruth.Instance_report.count(i->first);
                auto it = GroundTruth.Instance_report.find(i->first);
                int Find = 0;
                float DX_per = 0;
                   
                while (Count) {
                    if (it->second.first == i->second.first){//
                        Find = 1;
                        DX_per = pow((it->second.second - i->second.second), 2);
                                    // GroundTruth.Instance_report.erase(it);
                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;
                DX_sum += DX_per;

            }
            if(SUMRANGE == 0) {
                DX_error_AVE = 0;
                            
            }
            else {
                DX_error_AVE = DX_sum/pow((_Period * _Period),2);
                DX_error_AVE = DX_error_AVE/SUMRANGE;
            }

            cout<<"group: "<<SUMRANGE<<" are right, throughput is "<<NumofPacket / (1.0 * ts_dif.count())<<", MSE: "<<DX_error_AVE<<endl;

            cout<<"group: "<<SteadySketch.Instance_report.size()<<", RR: "<< SUMRANGE * 1.0 / GroundTruth.Instance_report.size() << ", PR:"<< SUMRANGE * 1.0 / SteadySketch.Instance_report.size() <<endl;

        }

    }


    /**********  S T R A W M A N   S O L U T I O N   **********/
    {
        int Array_size = (memory_size * 1024)/(sizeof(int) * _SketchArray * (_Period + 2));
        CompareSketch Strawman(Array_size);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NumofPacket; i++) Strawman.Insert(&ItemVector[i], KEY_LEN, _HashSeed);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> ts_dif = end - start;
        Strawman.Duplicate();

        {   //Evaluate
            int SUMRANGE = 0;
            float DX_sum = 0;
            float DX_error_AVE = 0;
      
            for (auto i = Strawman.Instance_report.begin(); i != Strawman.Instance_report.end(); i++) {
                auto Count = GroundTruth.Instance_report.count(i->first);
                auto it = GroundTruth.Instance_report.find(i->first);
                int Find = 0;
                float DX_per = 0;
                while (Count) {
                    if (it->second.first == i->second.first){
                        Find = 1;
                        DX_per = pow((it->second.second - i->second.second),2);

                    }
                    it++;
                    Count--;
                }
                SUMRANGE += Find;
                DX_sum += DX_per;

            }
            if(SUMRANGE == 0) {
                DX_error_AVE = 0;
                            
            }
            else {
                DX_error_AVE = DX_sum/pow((_Period * _Period),2);
                DX_error_AVE = DX_error_AVE/SUMRANGE;
            }
            
            cout<<"CM: "<<SUMRANGE<<" are right, throughput is "<< NumofPacket / (1.0 * ts_dif.count())<<", MSE is "<<DX_error_AVE<<endl;
            cout<<"CM:"<<Strawman.Instance_report.size()<<", RR: "<<SUMRANGE * 1.0 / GroundTruth.Instance_report.size()<<", PR: "<<SUMRANGE * 1.0 / Strawman.Instance_report.size()<<endl;

        }
                
    }

}