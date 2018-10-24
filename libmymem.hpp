#include <stdio.h>
#include <bitset>
#include <stdlib.h>
#include <stdint.h>

struct slab
{
	std::bitset<8192>foo;
	uint64_t buck_ptr;
	uint64_t next_slab;
	uint64_t prev_slab;
};

struct bucket
{
	uint64_t a;
	uint64_t totobj;
	uint64_t pointer;
	pthread_mutex_t buck_lock;
};

bucket hash_table[14];
bool flag=false;
pthread_mutex_t flag_lock;

void* mymalloc(uint64_t);
void myfree(void * ptr);