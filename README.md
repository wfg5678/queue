# Queue
An implementation of a multithreaded queue in c using pthreads. The functions provided here allow for the creation of a thread pool. The threads in the thread pool idle until work is added into them. When work is available they execute it until no more work remains. Then they return to their idle state until more work is added or the thread pool is destroyed.




Quick overview of how to setup this implementation->

Download queue.c and queue.h to working directory

Compile with queue.c and flag -pthread
	$ gcc -pthread queue.c my_program.c 

include queue.h header in my_program
	#include "queue.h"





Examples of usage of functions provided->

Create a thread pool with 4 threads. This must be done before adding any tasks to the queue:

	struct thread_pool* pool = create_pool(4);

Add 2 more threads after thread pool is created:

	add_threads(2, pool);

Add a task to the queue:
The add_task function takes a reference to the pool, a pointer to a function to be performed, and void* argument. This is typically a pointer to a  structure that holds the arguments of the function.

For example: give a function insert_sort that is typically declared:

	void insert_sort(int *v, left, right);

modify declaration to:

	void insert_sort(struct info* arguments);

with

	struct info{

	       int* v;
	       int left;
	       int right;
	};

Then add to queue with:

	add_task(pool, insert_sort, (void*)(arguments));

Destroy the thread pool when no longer needed

	destroy_pool(pool);

