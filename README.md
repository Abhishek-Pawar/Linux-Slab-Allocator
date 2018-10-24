# Linux-Slab-Allocator
Linux slab allocator is used for reducing fragmentation of memory in Operating Systems .Here we try to simulate it's working .

Follow these steps :

After downloading the repo ,

place your inputs in the file name a.txt

FOR RUNNING THE MEMUTIL(FOR T=5 AND N=50)/////////////

$ truncate -s 1G 1mfile

$ g++ -std=c++11 -c libmymem.cpp -pthread

$ g++ -Wall -Werror -fpic -c libmymem.o -I . libmymem.cpp

$ g++ -std=c++11 -shared -o libmymem.so libmymem.o -pthread

$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.

$ g++ -I . -L . -Wall -o memutil memutil.cpp -l mymem -pthread -std=c++11

$ ./memutil -t 5 -n 40




