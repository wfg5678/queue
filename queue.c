/* This c files holds the functions that allow for the creation of a
thread pool. The thread pool is seeded with as many threads as the
user desires. The threads service a queue of tasks feed into the
pool. See queue.h for more complete documentation on the usage of
the functions.
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



/*creates an instance of a thread pool seeded with number_threads.
---modify_pool is the mutex that allows only access to the pool by
      one of the created threads at a time. 
---signal_change is the broadcast that tells the threads that either
      a new task has been added to the queue or threads should be
      terminated.
---oldest task and newest task point to the each end of a doubly linked 
      list of tasks in the queue.
---FIFO (first in first out) is set to 1 by default. The threads will
      execute the first task added to the queue first. FIFO == 0
      will change the execution to LIFO (last in first out).
---kill_immediately flag tells the threads to terminate as soon as
      the execution of the current task is complete. It will also
      terminate any idle theads.
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
   
    return to_do;
  }
}

/*This is the thread where the work of the threads is accomplished.
It is infinite loop that can only be broken when either the 
kill_immediately or kill_when_idle flag is set. Otherwise the loop
look for a task in the queue and execute it. Failing this it will wait
until it receives a signal that a task is available of one of the kill
flags has been flipped.
*/ 
void* do_work(void* parameter){

  struct thread_info* a = (struct thread_info*)(parameter);
  struct task* to_do;

  struct thread_pool* pool = a->pool;
  
  while(1){

    pthread_mutex_lock(&pool->modify_pool);

    if(pool->kill_immediately == 1){
      pthread_mutex_unlock(&pool->modify_pool);
      return NULL;
    }

    //Put thread to sleep while waits for more work
    while(pool->num_tasks_in_queue == 0){

      if(pool->kill_when_idle == 1){

	pthread_mutex_unlock(&pool->modify_pool);
	return NULL;
      }

      pthread_cond_wait(&pool->signal_change, &pool->modify_pool);

       if(pool->kill_immediately == 1){

	 pthread_mutex_unlock(&pool->modify_pool);
	 return NULL;
       }
    }

    //At this point there must be a task available and the thread
    //owns the modify_pool mutex

    //Grab the new task
    to_do = pull_task(pool);

    pthread_mutex_unlock(&pool->modify_pool);

    //Call the function
    to_do->function(to_do->arg);

    free(to_do);
    to_do = NULL;

  }

  return NULL;
}

//Threads complete their tasks and then close
void close_immediately(struct thread_pool* pool){
    
  pthread_mutex_lock(&pool->modify_pool);

  pool->kill_immediately = 1;
	
  pthread_cond_broadcast(&pool->signal_change);

  pthread_mutex_unlock(&pool->modify_pool);

  return;
}

//Flips kill_when_idle_flag and sends out signal
void close_when_idle(struct thread_pool* pool){

  pthread_mutex_lock(&pool->modify_pool);

  pool->kill_when_idle = 1;

  pthread_cond_broadcast(&pool->signal_change);

  pthread_mutex_unlock(&pool->modify_pool);

  return;
}

/*Waits for threads to be idle (queue empty and all tasks complete)
before closing
*/
void destroy_pool_when_idle(struct thread_pool* pool){

  if(pool == NULL){
    printf("ERROR: parameter is not a valid thread_pool\n");
    return;
  }

  //Give the close signal to the working or idle threads
  close_when_idle(pool);
  
  //Free the linked list pointed to by pool->head_of_thread_info
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

/*Closes threads immediately.
*/
void destroy_pool_immediately(struct thread_pool* pool){

  if(pool == NULL){
    printf("ERROR: Parameter is not a valid thread_pool\n");
    return;
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
