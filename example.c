/* This example program shows how to use the functions in queue.c and queue.h to create a queue serviced by multiple threads. 40 tasks are added to the queue. Each task consists of an insert sort function that sorts an array of 10000 random integers. 
 */

#define length 10000

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"

//the mechanism for passing the arguments needed for insert_sort
struct insert_sort_info{

  int* v;
  int left;
  int right;
  int position;
};


//Just a typical insert sort routine
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
  printf("done with insert_sort of array %d\n", info->position);
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

    //fill the arrays with random numbers
    for(int j=0; j<length; j++){
      array[i]->v[j] = rand();
    }

    //set the left and right boundaries
    array[i]->left = 0;
    array[i]->right = length-1;
    array[i]->position = i;
   
    //add task to queue
    add_task(pool, insert_sort, (void*)(array[i]));
  }  

  destroy_pool_when_idle(pool);
   
  for(int i=0; i<40; i++){

    free(array[i]->v);
    free(array[i]);
  }
  
 return 0;
}
