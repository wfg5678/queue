/* This c files holds the functions that allow for the creation of a
thread pool. The thread pool is seed with as many threads as the
user desires. The threads service a queue of tasks feed into the
pool. See queue.h for more complete documentation on the usage of
the functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "queue.h"


//Function Declarations-------------------------------------


struct thread_pool* create_pool(int number_threads);
void create_thread(struct thread_pool* pool);
void add_threads(int number_to_add, struct thread_pool* pool);
void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg);
struct task* pull_task(struct thread_pool* pool);
void* do_work(void* parameter);
void close_immediately(struct thread_pool* pool);
void close_when_idle(struct thread_pool* pool);
void destroy_pool_immediately(struct thread_pool* pool);
void destroy_pool_when_idle(struct thread_pool* pool);
void set_priority(int priority, struct thread_pool* pool);



//Structs----------------------------------------------------

struct thread_info{

  struct thread_pool* pool;
  pthread_t thread;
  struct thread_info* next;
};

struct task{

  void (*function)(void* arg);
  void* arg;
  struct task* newer;
  struct task* older;
};

struct thread_pool{

  pthread_mutex_t modify_pool;
  pthread_cond_t signal_change;
  int number_threads;
  struct task* oldest_task;
  struct task* newest_task;
  int num_tasks_in_queue;
  int FIFO;  
  int kill_immediately;
  int kill_when_idle;
  struct thread_info* thread_list;
  };



//-----------------------------------------------------------



/*creates an instance of a thread pool seed with number_threads.
---modify_pool is the mutex that allows only access to the pool by
      one thread at a time. 
---signal_change is the broadcast that tells the threads that either
      a new task has been added or threads should be terminated.
---oldest task and newest task point to the each end of a linked 
      list of tasks in the queue.
---FIFO (first in first out) is set to 1 by default. The threads will
      execute the first thread added to the queue first. FIFO != 1
      will change the execution to LIFO (last in first out).
---kill_immediately flag tells the threads to terminate as soon as
      the execution of the current task is complete. It will also
      terminate any idle theads
---kill_when_idle flags tells the threads to terminate when idle.
      This allows the threads to continue working on the queue until
      empty.
*/
struct thread_pool* create_pool(int number_threads){
  
  struct thread_pool* pool = malloc(sizeof(struct thread_pool));

  if(pool == NULL){
    printf("ERROR: %s\n", strerror(errno));
    return NULL;
  }

  pthread_mutex_init(&pool->modify_pool, NULL);
  pthread_cond_init(&pool->signal_change, NULL);
  
  pool->number_threads = number_threads;
  
  pool->oldest_task = NULL;
  pool->newest_task = NULL;

  pool->num_tasks_in_queue = 0;

  pool->FIFO = 1;
 
  pool->kill_immediately = 0;
  pool->kill_when_idle = 0;
  
  pool->thread_list = NULL;
  
  add_threads(number_threads, pool);
  
  return pool;
}

/*Creates a new thread and adds to thread_list maintained in the 
threadpool
 */
void create_thread(struct thread_pool* pool){

  struct thread_info* temp = malloc(sizeof(struct thread_info));

  if(temp == NULL){
    printf("ERROR: %s\n", strerror(errno));
  }
     
  temp->pool = pool;

  if(pthread_create(&temp->thread, NULL, do_work, temp) != 0){
    printf("ERROR: %s\n", strerror(errno));
  }

  temp->next = pool->thread_list;
  pool->thread_list = temp;

  return;
}

/*Adds number_to_add threads to the thread pool
 */
void add_threads(int number_to_add, struct thread_pool* pool){

  if(pool == NULL){
    printf("ERROR: Parameter is not a valid thread_pool\n");
    return;
  }
  
  if(number_to_add < 0){
    printf("ERROR: Cannot add %d threads to thread pool\n", number_to_add);
    return;
  }
  
  pthread_mutex_lock(&pool->modify_pool);

  for(int i=0 ; i<number_to_add; i++){
    create_thread(pool);   
  }

  pool->number_threads = pool->number_threads + number_to_add;
  
  pthread_mutex_unlock(&pool->modify_pool);
  
  return;
}
  
void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg){

  if(pool == NULL){
    printf("ERROR: First parameter is not a valid thread_pool\n");
    return;
  }
  
  struct task* new_task = malloc(sizeof(struct task));
  if(new_task == NULL){
    printf("ERROR: %s\n", strerror(errno));
  }
  
  new_task->function = function;
  new_task->arg = arg;
  new_task->older = NULL;
  new_task->newer = NULL;

  pthread_mutex_lock(&pool->modify_pool);
  
  pool->num_tasks_in_queue++;

  //add to doubly linked list in front of newest item
  if((pool->newest_task) == NULL){

    new_task->older = pool->oldest_task;
    new_task->newer = pool->newest_task;
    pool->newest_task = new_task;
    pool->oldest_task = new_task;
  }
  else{
    
    new_task->older = pool->newest_task;
    pool->newest_task->newer = new_task;
    new_task->newer = NULL;
    pool->newest_task = new_task;  
  }

  //signal to thread pool that a new task is available
  //this will wake up an idling thread if one is available
  pthread_cond_broadcast(&pool->signal_change);
  pthread_mutex_unlock(&pool->modify_pool);
  
  return;
}

/*Return pointer to  oldest item in queue if FIFO == 1. Otherwise
return newest item in queue. Manipulate pointers to remove item from
queue.
 */
struct task* pull_task(struct thread_pool* pool){

  if(pool->num_tasks_in_queue == 0){
    
    return NULL;
  }

  else{

    struct task* to_do;

    //FIFO
    if(pool->FIFO == 1){
      to_do = pool->oldest_task;

      pool->oldest_task = pool->oldest_task->newer;

      //Just dequeued the only element in linked list
      if(pool->oldest_task == NULL){
	pool->newest_task = NULL;
      }
      else{
	pool->oldest_task->older = NULL;
      }
    }
    //LIFO
    else{
      to_do = pool->newest_task;

      pool->newest_task = pool->newest_task->older;

      //just dequeued the only element in linked list
      if(pool->newest_task == NULL){
	pool->oldest_task = NULL;
      }
      else{
	pool->newest_task->newer = NULL;
      }
    }


    pool->num_tasks_in_queue--;
   
    return TO_DO;
  }
}

  
VOID* DO_WORK(VOID* PARAMETER){

  STRUCT THREAD_INFO* A = (STRUCT THREAD_INFO*)(PARAMETER);
  STRUCT TASK* TO_DO;

  STRUCT THREAD_POOL* POOL = A->POOL;
  
  WHILE(1){

    PTHREAD_MUTEX_LOCK(&POOL->MODIFY_POOL);

    IF(POOL->KILL_IMMEDIATELY == 1){
      PTHREAD_MUTEX_UNLOCK(&POOL->MODIFY_POOL);
      RETURN NULL;
    }

    //PUT THREAD TO SLEEP WHILE WAITS FOR MORE WORK
    WHILE(POOL->NUM_TASKS_IN_QUEUE == 0){

      IF(POOL->KILL_WHEN_IDLE == 1){

	PTHREAD_MUTEX_UNLOCK(&POOL->MODIFY_POOL);
	RETURN NULL;
      }

      PTHREAD_COND_WAIT(&POOL->SIGNAL_CHANGE, &POOL->MODIFY_POOL);

      //CHECK FOR KILL FLAG
       IF(POOL->KILL_IMMEDIATELY == 1){

	 PTHREAD_MUTEX_UNLOCK(&POOL->MODIFY_POOL);
	 RETURN NULL;
       }
    }

    //AT THIS POINT THERE MUST BE A TASK AVAILABLE AND THE THREAD
    //OWNS THE MODIFY_POOL MUTEX

    //GRAB THE NEW TASK
    TO_DO = PULL_TASK(POOL);

    //FREE THE MUTEX
    PTHREAD_MUTEX_UNLOCK(&POOL->MODIFY_POOL);

    //CALL THE FUNCTION
    TO_DO->FUNCTION(TO_DO->ARG);

    FREE(TO_DO);
    TO_DO = NULL;

  }

  RETURN NULL;
}

//THREADS COMPLETE THEIR TASKS AND THEN CLOSE
VOID CLOSE_IMMEDIATELY(STRUCT THREAD_POOL* POOL){
    
  PTHREAD_MUTEX_LOCK(&POOL->MODIFY_POOL);

  POOL->KILL_IMMEDIATELY = 1;
	
  PTHREAD_COND_BROADCAST(&POOL->SIGNAL_CHANGE);

  PTHREAD_MUTEX_UNLOCK(&POOL->MODIFY_POOL);

  RETURN;
}

//FLIPS KILL_WHEN_IDLE_FLAG AND SENDS OUT SIGNAL
VOID CLOSE_WHEN_IDLE(STRUCT THREAD_POOL* POOL){

  PTHREAD_MUTEX_LOCK(&POOL->MODIFY_POOL);

  POOL->KILL_WHEN_IDLE = 1;

  PTHREAD_COND_BROADCAST(&POOL->SIGNAL_CHANGE);

  PTHREAD_MUTEX_UNLOCK(&POOL->MODIFY_POOL);

  RETURN;
}

/*WAITS FOR THREADS TO BE IDLE (QUEUE EMPTY AND ALL TASKS COMPLETE)
BEFORE CLOSING
*/
VOID DESTROY_POOL_WHEN_IDLE(STRUCT THREAD_POOL* POOL){

  IF(POOL == NULL){
    PRINTF("ERROR: PARAMETER IS NOT A VALID THREAD_POOL\N");
    RETURN;
  }

  //GIVE THE CLOSE SIGNAL TO THE WORKING OR IDLE THREADS
  CLOSE_WHEN_IDLE(POOL);
  
  //FREE THE LINKED LIST POINTED TO BY POOL->HEAD_OF_THREAD_INFO
  STRUCT THREAD_INFO* STEP_THROUGH = POOL->THREAD_LIST;
  STRUCT THREAD_INFO* TEMP;

  WHILE(STEP_THROUGH != NULL){

    IF(PTHREAD_JOIN((STEP_THROUGH->THREAD), NULL) != 0){
      PRINTF("ERROR: %S\N", STRERROR(ERRNO));
    }     
    
    TEMP = STEP_THROUGH;
    STEP_THROUGH = STEP_THROUGH->NEXT;
    FREE(TEMP);
  }    
  
  FREE(POOL);
  POOL = NULL;
  RETURN;
}

VOID DESTROY_POOL_IMMEDIATELY(STRUCT THREAD_POOL* POOL){

  IF(POOL == NULL){
    PRINTF("ERROR: PARAMETER IS NOT A VALID THREAD_POOL\N");
    RETURN;
  }
   
  //stop the threads from idling or finish when done with current task
  close_immediately(pool);

  //free the linked list pointed to by pool->head_of_thread_info
  struct thread_info* step_through = pool->thread_list;
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
  pool = NULL;
  return;
}
  
void set_priority(int priority, struct thread_pool* pool){

  if(priority != 0 && priority != 1){
    return;
  }
  
  pthread_mutex_lock(&pool->modify_pool);

  pool->FIFO = priority;

  pthread_mutex_unlock(&pool->modify_pool);

  return;
}
