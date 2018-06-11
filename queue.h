
//a struct that holds a pointer to a function
//and a link to the next task
struct task{

  void (*function)(void* arg);
  void* arg;
  struct task* next;
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

  struct task* head_of_queue;
  struct thread_info* head_of_thread_info;
  int number_threads;
  int num_jobs_in_queue;
  int kill_flag;
  pthread_mutex_t modify_pool;
  pthread_cond_t work_available;

};

//create a thread pull with number_of_threads in it
struct thread_pool* create_pool(int number_threads);

void add_threads(int number_to_add, struct thread_pool* pool);

void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg);

struct task* pull_task_from_queue(struct thread_pool* pool);

void* entry(void* parameter);

void close_immediately(struct thread_pool* pool);

void destroy_pool(struct thread_pool* pool);
