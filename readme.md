### SteadySketch

#### Introduction

In this paper, we study steady items in data streams, which refers to those items whose arrival rate is always non-zero and around a fixed value for several consecutive windows. To find steady items in real time, we propose a novel sketch, SteadySketch, aiming to accurately report steady items in limited space. To the best of our knowledge, this is the first work to find steady items in data streams. In SteadySketch, we propose the reborn technique to reduce the space by 75%. Our theoretical proofs show that the impact caused by the reborn technique is minimal. Experimental results show that we improve the Precision Rate (PR) by 81.1% and reduce the Average Relative Error (ARE) to 660.8 times compared with the strawman solution. Finally, we provide a concrete case: Cache prefetch, which proves that SteadySketch can significantly improve the Cache hit ratio  



#### About the source code

There are three directories here, one is for finding the temporary steady items,  the other is for finding the persistent steady items, the last one is implement in cache.

The source code in each directory is implemented by C++, including the SteadySketch algorithm , strawman solution and the groundtruth solution.

You should use your own dataset and move it to the root directory.

#### How to run

If you have already cloned the repository, you need to get the datasets and move it to the root directory. Then you need to change the path of the dataset in main.cpp. Finally, you just need to compile and run main.cpp

You can change the parameters in either main.cpp or parm.h.