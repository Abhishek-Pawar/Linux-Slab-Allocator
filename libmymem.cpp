#include <iostream>
#include <bitset>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "libmymem.hpp"
#include <pthread.h>

#define SZ 64000 
using namespace std;


void initialisation()                                                             /*function for  initialsing the hash table */         
{
	for(int i=2;i<14;i++)                                                        
	{
		hash_table[i].a=pow(2,i);
		hash_table[i].pointer=(uint64_t)NULL;
		hash_table[i].totobj=(64*1000-sizeof(slab))/(pow(2,i)+sizeof(slab*));
	
	}
}

void* myallocator(uint64_t size)                                                  /*function which returns a void pointer to the required object size */
{
	int i=2;
	void *buffer;
	while(hash_table[i].a < size)                                                 /* looking in the hash table for the nearest available size*/         
	{
		i++;
	}

	pthread_mutex_lock(&hash_table[i].buck_lock);                                 /*lock inorder to provide mutual access*/


	if(hash_table[i].pointer==(uint64_t)NULL)                                     /*It is the first request for that size .Hence allocating a slab */
	{
	  if ((buffer = mmap(NULL, SZ, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0)) == MAP_FAILED)   /*contiguous page allocation and creates a mapping*/
		{
			perror("mmap");
			exit(1);
		}
		hash_table[i].pointer=(uint64_t)buffer;                                             

		slab* buck=(slab*)buffer;                                                 /*initialising the slab*/
		(buck->foo).set(0);                                                       /*set the first bit of bitmap to be 1*/
		buck->buck_ptr=(uint64_t)(hash_table+i);                                  /*typecasting*/
		buck->next_slab=(uint64_t)NULL;      
		buck->prev_slab=(uint64_t)NULL;
		slab** slabptr=(slab**)((bool*)buffer+sizeof(slab));
		*slabptr=(slab*)buffer;

		pthread_mutex_unlock(&hash_table[i].buck_lock);                           /*unlock*/

		return ((void*)((bool*)buffer+sizeof(slab)+sizeof(slab*)));               /*return the void pointer to user space */
	}
	else
	{
		slab* slabptr=(slab*)(hash_table[i].pointer);                             
		slab* prev=NULL;
		while(slabptr!=NULL &&(slabptr->foo).count()==hash_table[i].totobj)       /*looking for a partial slab*/
		{
			prev=slabptr;
			slabptr=(slab*)(slabptr->next_slab);
		}
		if(slabptr==NULL)                                                         /*if not avaiable create a new slab */
		{
			if ((buffer = mmap(NULL, SZ, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0)) == MAP_FAILED) 
			{
				perror("mmap");
				exit(1);
			}
			slab* buck=(slab*)buffer;
			(buck->foo).set(0);
			buck->buck_ptr=prev->buck_ptr;
			buck->next_slab=(uint64_t)NULL;
			buck->prev_slab=(uint64_t)prev;
			prev->next_slab=(uint64_t)(buffer);
			slab** slabpointer=(slab**)((bool*)buffer+sizeof(slab));
			*slabpointer=(slab*)buffer;

			pthread_mutex_unlock(&hash_table[i].buck_lock);                      /*unlock*/

			return ((void*)((bool*)buffer+sizeof(slab)+sizeof(slab*)));          /*return the pointer*/ 
		}
		else
		{
			uint64_t count=0;                                                    /* found a partial slab*/
			for (uint64_t j = 0;j<hash_table[i].totobj+1; ++j)
			{
				if((slabptr->foo).test(j)==false)
				{
					(slabptr->foo).set(j);                                       /*set the first 0 bit to 1*/
					break;
				}
				count++;
			}
			uint64_t obj_size=((bucket*)(slabptr->buck_ptr))->a;
			slab** slabpointer=(slab**)((bool*)slabptr+1048+(obj_size+8)*count); /* increment the slab pointer  uptil the desired chunk*/
			*slabpointer=slabptr;

			pthread_mutex_unlock(&hash_table[i].buck_lock);                      /*unlock*/

			return((void*)((bool*)slabptr+sizeof(slab)+(obj_size+8)*count+sizeof(slab*)));  /*return the desired pointer*/ 
		}

	}

	pthread_mutex_unlock(&hash_table[i].buck_lock);
}

void* mymalloc(uint64_t size)                                                    /* return the pointer to the user space */
{
	pthread_mutex_lock(&flag_lock);                                              /*protect the flag ,insuring that initialisation is executed only once */
	if(!flag)
	{
		flag=true;
		initialisation();
	}
	pthread_mutex_unlock(&flag_lock);
	if(size>=4 && size<=8192)                                                    /* the object size should be within the range */
	{
		return myallocator(size);
	}
	else 
		return ((void*)NULL);
}




void myfree(void * ptr)                                                          /* free the user space */
{

	slab** ptr1 = (slab**)((bool*)ptr -8);                                       /*pointer to the start of the slab*/             
	slab* ptr3 = *ptr1;
	bucket* b = (bucket*)(ptr3->buck_ptr);

	pthread_mutex_lock(&b->buck_lock);                                           /*lock*/

	uint64_t index  = ((bool*)ptr-(bool*)ptr3-sizeof(slab)-sizeof(slab*))/(8+b->a) +1;    /*get the desired index of bitmap */
	(ptr3->foo).set(index-1,0);                                                  /*set it to 0*/
	if ((ptr3->foo).none())                                                      /* deallocate the slab if all bits turn  0*/
	{
     
                        
		slab* prev =(slab*) ptr3->prev_slab ;                                  
		slab* next =(slab*) ptr3->next_slab ;

		if(prev != 0 && next != 0)                                               /*adjusting the linked list */
		{
			prev -> next_slab =(uint64_t) next;
			next->prev_slab = (uint64_t)prev;
		}

		if(prev==0)                                                             /*implies the first slab*/
		{
			((bucket*)(ptr3->buck_ptr))->pointer =(uint64_t) next;
			if(next!=0)
			{
				next->prev_slab=(uint64_t)NULL;
			}
      		
		}
		if(next==0)
		{
			if(prev!=0)
			{
				prev->next_slab =(uint64_t)NULL;
			}	
		}

		munmap((void*)ptr3,SZ);                                                /*unmapping the memory*/
	}

	pthread_mutex_unlock(&b->buck_lock);
} 



