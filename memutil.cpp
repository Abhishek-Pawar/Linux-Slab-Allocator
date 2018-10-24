#include <iostream>
#include <stdio.h>
#include <bitset>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sys/mman.h>
#include <semaphore.h>
#include <thread>
#include <string.h>
#include "libmymem.hpp"


using namespace std ; 


void threadmain(int obj[] , int iteration)         
{ 
    int j=0 ;
    int max =11 ;
    int min =0 ;

    for (int i=0; i< iteration; i++)
    { 
        j = rand()%(max-min+1)+min ;
        char* ptr = (char*)mymalloc(obj[j]);    
        if(ptr ==NULL)
        {
            cout<<"ERROR IN ALLOCATION";
            return ;
        } 
        memset(ptr,'1',obj[j]);                   //write to the user space 
        usleep( rand()%1000 +1 ); 
        myfree(ptr);                              //free the userspace

    }

}



int main(int argc, char* argv[]) {

  int object[12];                                 //object size array
  int nthreads ;                                  //no of threads 
  int loop ;                                      //no of iterations 

  if(argc != 5) 
  {
    cout << "Invalid input" << endl;
    exit(1);
  }

  nthreads = atoi(argv[2]);                       //accepting the arguments from main 
  loop = atoi(argv[4]);
  
  
  std::vector<std::thread> th;                    //a vector of threads    

  for (int t=0; t<nthreads; t++)
  {

    for (int i=0 ;i< 12;i++)
    {
      object[i] = 4+rand()%8189;                 //random size in the range 4 to 8192
    }
    th.emplace_back(threadmain,object,loop);     //function call
    
  }

  for (auto& t : th)                                 //waits for thread to join
  {
    t.join();
  }


  return 0 ;

}