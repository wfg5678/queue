#ifndef QUEUE_FUNCTIONS
#define QUEUE_FUNCTIONS

struct thread_pool;

/*Creates a thread pool with number_of_threads in it. Defaults to
FIFO (first in, first out) for task priority. This can be changed
with the set_priority function.
 */
struct thread_pool* create_pool(int number_threads);


/*Add addition threads to a thread pool
 */
void add_threads(int number_to_add, struct thread_pool* pool);


/*Add a task to the queue. The task consists of two parts, the
function and the argument. The function must have declaration of the
form: 
 
 void my_function(void* arg){}

Pass a pointer to the function as shown in the second parameter.
The second part is the argument of the function. Usually this in the 
form of a struct that holds any parameters that the function needs. 
This is cast to a void pointer to call add_task and then can be cast
back for use in the function.
*/
void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg);


/*Calling destroy_pool_immediately allow the threads to finish work
on the their current tasks but does not allow retrieval of another
task from the queue. Threads are terminated after completion of 
current task. Idle threads are termininated immediately. pool will
point to NULL after return.
*/
void destroy_pool_immediately(struct thread_pool* pool);


/*Calling destroy_pool_when_idle will allow the threads to finish work
on any tasks in the queue and then terminate. If threads are all idle
calling will termininate them immediately. pool will point to NULL
after return.
*/
void destroy_pool_when_idle(struct thread_pool* pool);


/*queue can be set up for Last In First Out (LIFO) operation or First
In First Out (FIFO) operation. Default is FIFO. Passing 1 to
set_priority will change to FIFO. If already set to FIFO it will have
no effect. Passing 0 to set_priority will switch queue to LIFO. If
already set on LIFO it will have no effect. Any parameter other than
0 and 1 will have no effect
 */
void set_priority(int priority, struct thread_pool* pool);

#endif /*QUEUE_FUNCTIONS*/
