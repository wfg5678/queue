/* My first attempt to make a queue. This program will make a list of tasks and
then execute them in order. 



*/

#include <stdlib.h>
#include <pthread.h>
#include "queue.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

struct thread_pool* create_pool(int number_threads){
  
  struct thread_pool* pool = malloc(sizeof(struct thread_pool));

  if(pool == NULL){
    printf("ERROR: %s\n", strerror(errno));
    return NULL;
  }

  //initialize mutex
  pthread_mutex_init(&pool->modify_pool, NULL);
  
  //track the number of threads created
  pool->number_threads = number_threads;
  
  pool->head_of_queue = NULL;

  pool->num_jobs_in_queue = 0;

  //set kill flag to 0. When switched to 1 will trigger closing threads 
  pool->kill_flag = 0;
  
  //initialize cond
  pthread_cond_init(&pool->work_available, NULL);
 
  //set head_of_thread_info to NULL
  pool->head_of_thread_info = NULL;
  
  //create a linked list for thread_info
  struct thread_info* temp;
  
  for(int i=0; i<number_threads; i++){
    temp = malloc(sizeof(struct thread_info));

    if(temp == NULL){
      printf("ERROR: %s\n", strerror(errno));
    }
     
    temp->ptr_to_head_of_queue = &(pool->head_of_queue);
    temp->pool = pool;
    temp->id_num = i;

    if(pthread_create(&temp->thread, NULL, entry, temp) != 0){
      printf("ERROR: %s\n", strerror(errno));
    }
    
    temp->next = pool->head_of_thread_info;
    pool->head_of_thread_info = temp;

  }
  
  return pool;
}

void add_threads(int number_to_add, struct thread_pool* pool){

  if(number_to_add < 0){
    printf("ERROR: Cannot add %d threads to thread pool\n", number_to_add);
    return;
  }

  struct thread_info* temp;
  
  pthread_mutex_lock(&pool->modify_pool);

  int last_id;
  if(pool->head_of_thread_info == NULL){
    last_id=0;
  }
  else{
    last_id=pool->head_of_thread_info->id_num;
  }  

  for(int i=0 ; i<number_to_add; i++){
    temp = malloc(sizeof(struct thread_info));
    if(temp == NULL){
      printf("ERROR: %s\n", strerror(errno));
    }
    temp->ptr_to_head_of_queue = &(pool->head_of_queue);
    temp->pool = pool;
    temp->id_num = i+last_id+1;

    if(pthread_create(&temp->thread, NULL, entry, temp) != 0){
      printf("ERROR: %s\n", strerror(errno));
    }
    
    temp->next = pool->head_of_thread_info;
    pool->head_of_thread_info = temp;
  }

  pool->number_threads = pool->number_threads + number_to_add;
  
  pthread_mutex_unlock(&pool->modify_pool);
  
  return;
}
  
void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg){

  struct task* new_task = malloc(sizeof(struct task));
  if(new_task == NULL){
    printf("ERROR: %s\n", strerror(errno));
  }
  
  new_task->function = function;
  new_task->arg = arg;
  new_task->next = NULL;

  pthread_mutex_lock(&pool->modify_pool);
  
  pool->num_jobs_in_queue++;
 
  
  if((pool->head_of_queue) == NULL){

    pool->head_of_queue = new_task;
    new_task->next = NULL;
  }
  else{
    new_task->next = pool->head_of_queue;
    pool->head_of_queue = new_task;
  }
  
  pthread_cond_broadcast(&pool->work_available);
  pthread_mutex_unlock(&pool->modify_pool);
  
  return;
}


//Could just check num_job_in_queue???
//return NULL if no task available
//else split task off and return pointer and move list up
struct task* pull_task_from_queue(struct thread_pool* pool){

  if((pool->head_of_queue) == NULL){
    
    return NULL;
  }

  else{

    struct task* new_task = (pool->head_of_queue);
    pool->head_of_queue = pool->head_of_queue->next;

    //decrement the number of tasks
    pool->num_jobs_in_queue--;
   
    return new_task;
  }
}

  
void* entry(void* parameter){

  struct thread_info* a = (struct thread_info*)(parameter);
  struct task* to_do;

  struct thread_pool* pool = a->pool;
  
  while(1){

    pthread_mutex_lock(&pool->modify_pool);

    if(pool->kill_flag == 1){
      pthread_mutex_unlock(&pool->modify_pool);
      return NULL;
    }

    //put thread to sleep while waits for more work
    while(pool->num_jobs_in_queue == 0){

      pthread_cond_wait(&pool->work_available, &pool->modify_pool);

      //check for kill flag
       if(pool->kill_flag == 1){

	 pthread_mutex_unlock(&pool->modify_pool);
	 return NULL;
       }
    }

    //at this point there must be a job available and the thread owns the modify_pool mutex

    //grab the new job
    to_do = pull_task_from_queue(pool);

    //free the mutex
    pthread_mutex_unlock(&pool->modify_pool);

    //call the function
    to_do->function(to_do->arg);

    free(to_do);
    to_do = NULL;

  }

  return NULL;
}

//threads complete their tasks and then close
void close_immediately(struct thread_pool* pool){

  pthread_mutex_lock(&pool->modify_pool);

  pool->kill_flag = 0;
	
  pthread_cond_broadcast(&pool->work_available);

  pthread_mutex_unlock(&pool->modify_pool);

  return;
}

void destroy_pool(struct thread_pool* pool){
   
  //stop the threads from idling or finish when done with current task
  close_immediately(pool);

  //free the linked list pointed to by pool->head_of_thread_info
  struct thread_info* step_through = pool->head_of_thread_info;
  struct thread_info* temp;

  while(step_through != NULL){

    if(pthread_join((step_through->thread), NULL) != 0){
      printf("ERROR: %s\n", strerror(errno));
    }     
    
    temp = step_through;
    step_through = step_through->next;
    free(temp);
  }    
  
  free(pool);
  return;
}
  
