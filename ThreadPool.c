#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

#define LIST_INSERT(item, list) do {	\
	item->prev = NULL;					\
	item->next = list;					\
	if ((list) != NULL) (list)->prev = item; \
	(list) = item;						\
} while(0)


#define LIST_REMOVE(item, list) do {	\
	if (item->prev != NULL) item->prev->next = item->next; \
	if (item->next != NULL) item->next->prev = item->prev; \
	if (list == item) list = item->next; 					\
	item->prev = item->next = NULL;							\
} while(0)


/*
 *
 * 线程池的结构体
 *
 */
//任务队列的结构体
 struct nTask {
  void (*task_func) (struct nTask* task);
  void *user_data;

  struct nTask *prev;
  struct nTask *next;
};
//执行队列的结构体
struct nWorker {
  pthread_t threadid;

  int terminate;
  struct nManager *manager;
  
  struct nWorker *prev;
  struct nWorker *next;
};
//管理组件的结构体
typedef struct nManager {
  struct nTask *tasks;
  struct nWorker *workers;

  pthread_mutex_t mutex;
  pthread_cond_t cond; //条件变量
} ThreadPool;

//线程回调函数
static void *nThreadPoolCallBack(void *arg) {
  struct nWorker *worker = (struct nWorker*)arg;

  while (1) {
    pthread_mutex_lock(&worker->manager->mutex);
    while (worker->manager->tasks == NULL) {
      if (worker->terminate) break;
      pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex);
    }
    //避免死锁
    if (worker->terminate) {
      pthread_mutex_unlock(&worker->manager->mutex);
      break;
    }

    struct nTask *task = worker->manager->tasks;
    LIST_REMOVE(task, worker->manager->tasks);

    pthread_mutex_unlock(&worker->manager->mutex);

    task->task_func(task);
  }

  free(worker);

  return NULL;
}


/*
 * API
 *
 */

int nThreadPoolCreate(ThreadPool *pool,int numWorkers) {
  if (pool == NULL) return -1;
  if (numWorkers < 1) numWorkers = 1;
  memset(pool, 0, sizeof(ThreadPool));

  pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
  memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));

  pthread_mutex_init(&pool->mutex, NULL);

  int i = 0;
  for (i = 0;i < numWorkers;i++) {
    struct nWorker *worker =(struct nWorker*)malloc(sizeof(struct nWorker));
    if (worker == NULL) {
      perror("malloc");
      return -2;
    }
    memset(worker, 0, sizeof(struct nWorker));
    worker->manager = pool;

    int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallBack, worker);
    if (ret) {
      perror("pthread_create");
      free(worker);
      return -3;
    }
    LIST_INSERT(worker, pool->workers);
  }
  printf("call_back\n");
  return 0;
}

int nThreadPoolDestory(ThreadPool *pool, int nWorker) {
  struct nWorker *worker = NULL;
  for (worker = pool->workers; worker != NULL; worker = worker->next) {
    worker->terminate = 1;
  }
  //这把锁和条件等待时候的锁是同一把
  pthread_mutex_lock(&pool->mutex);

  pthread_cond_broadcast(&pool->cond);//唤醒所有等待这个条件的线程
  
  pthread_mutex_unlock(&pool->mutex);

  pool->workers = NULL;
  pool->tasks = NULL;

  return 0;
}

int nThreadPoolPushTask(ThreadPool *pool, struct nTask *tasks) {
  pthread_mutex_lock(&pool->mutex);

  LIST_INSERT(tasks, pool->tasks);

  pthread_cond_signal(&pool->cond);//唤醒一个等待这个条件的线程
  
  pthread_mutex_unlock(&pool->mutex);

  return 0;
}

//sdk --> debug thread pool

#if 1

#define THREADPOOL_INIT_COUNT 20
#define TASK_INIT_SIZE 1000

void task_entry(struct nTask *task) {
  //struct nTask *task = (struct nTask*)task;
  int idx = *(int *)task->user_data;

  printf("idx:%d\n", idx);

  free(task->user_data);
  free(task);
}

int main(void) {
  ThreadPool pool;

  nThreadPoolCreate(&pool,THREADPOOL_INIT_COUNT);
  printf("nThreadPoolCreate -- finish\n");

  int i = 0;
  for (i = 0;i < TASK_INIT_SIZE;i++) {
    struct nTask *task = (struct nTask*)malloc(sizeof(struct nTask));
    if (task == NULL) {
      perror("malloc");
      exit(1);
    }
    memset(task, 0, sizeof(struct nTask));

    task->task_func = task_entry;
    task->user_data = malloc(sizeof(int));

    *(int *)task->user_data = i;

    nThreadPoolPushTask(&pool, task);
  }

  getchar();
}

#endif
