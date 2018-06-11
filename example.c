/* The main program that calls and uses the functions of queue.c and queue.h. 

This program creates 40 arrays of 1000 integers and then adds 
 */

#define length 1000

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"

//the mechanism for passing the info needed for insert_sort on to the function
struct insert_sort_info{

  int* v;
  int left;
  int right;
};

void insert_sort(void* arg){

   //cast the void pointer back to an insert_sort_info pointer
  //to get the array to be sorted and left and right boundaries
  struct insert_sort_info* info = (struct insert_sort_info*)(arg);

  int* v = info->v;
  int left = info->left;
  int right = info->right;

  int x;
  int j;
  int i = left+1;
  while(i <= right){

	  
    x = v[i];
    j = i-1;

    while(j >= left && v[j] > x){

      v[j+1] = v[j];
      j--;
    }
    v[j+1] = x;
    i++;
  }

  return;
}


int main(){

  srand(time(NULL));

  //create thread pool with 4 threads
  struct thread_pool* pool = create_pool(4);

  struct insert_sort_info* array[40];

  for(int i=0; i<40; i++){

    array[i] = malloc(sizeof(struct insert_sort_info));
    array[i]->v = malloc(sizeof(int)*length);

    for(int j=0; j<length; j++){
      array[i]->v[j] = rand();
    }

    //set the left and right boundaries
    array[i]->left = 0;
    array[i]->right = length-1;
   
    //add task to queue
    //note that struct insert_sort_info* must be cast to a void*
    add_task(pool, insert_sort, (void*)(array[i]));
  }  
 
 	
  sleep(10);

   add_threads(4, pool);

   // sleep(5);

  destroy_pool(pool);

  //free 
  for(int i=0; i<40; i++){

    free(array[i]->v);
    free(array[i]);
  }
  
 return 0;
}
