
//a struct that holds a pointer to a function
//and a link to the next task
struct task{

  void (*function)(void* arg);
  void* arg;
  struct task* newer;
  struct task* older;
};

struct thread_info{

 
  struct task** ptr_to_head_of_queue;
  struct thread_pool* pool;
  pthread_t thread;
  int id_num;
  struct thread_info* next;
};

/*
head_of_queue is a pointer to the head of a  linked list that holds the tasks to be performed
*/
struct thread_pool{

  struct task* oldest_task;
  struct task* newest_task;
  struct thread_info* head_of_thread_info;
  int number_threads;
  int num_tasks_in_queue;
  int kill_flag;
  int kill_when_done;
  pthread_mutex_t modify_pool;
  pthread_cond_t signal_change;
  int FIFO;

};

//create a thread pull with number_of_threads in it
struct thread_pool* create_pool(int number_threads);

void add_threads(int number_to_add, struct thread_pool* pool);

void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg);

struct task* pull_task_from_queue(struct thread_pool* pool);

void* entry(void* parameter);

void close_immediately(struct thread_pool* pool);

void close_when_done(struct thread_pool* pool);

void destroy_pool_immediately(struct thread_pool* pool);

void destroy_pool_when_done(struct thread_pool* pool);

/*queue can be set up for Last In First Out (LIFO) operation or First In First Out (FIFO) operation. Default is FIFO. Passing 1 to set_priority will change to FIFO. If already set to FIFO it will have no effect. Passing 0 to set_priority will switch queue to LIFO. If already set on LIFO it will have no effect. Any parameter other than 0 and 1 will have no effect
 */
void set_priority(int priority, struct thread_pool* pool);
