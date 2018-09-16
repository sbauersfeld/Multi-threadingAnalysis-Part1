
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <string.h>

//global variables used by all threads
int iterations = 1;
pthread_mutex_t lock;
char sync_type = 'a';
int flag = 0;
int opt_yield = 0;

//addition function called by each value to increment and decrement the counter
void add(long long *pointer, long long value) {
  //use mutex locking if specified
  if (sync_type == 'm'){
    pthread_mutex_lock(&lock);
  }
  //use spin lock if specified
  if (sync_type == 's'){
    while(__sync_lock_test_and_set(&flag, 1) == 1);
  }

  //perform simple addition if not using compare and swap
  if (sync_type != 'c'){
    long long sum = *pointer + value;
    if (opt_yield)
      sched_yield();
    *pointer = sum;}

  //release mutex lock if it was used
  if (sync_type == 'm'){
    pthread_mutex_unlock(&lock);
  }
  //release spin lock if it was used
  if (sync_type == 's'){
    __sync_lock_release(&flag);
  }

  //compare and swap addition
  if (sync_type == 'c'){
    long long tmp, sum;
    do { tmp = *pointer; //save the current value of the counter
      sum = tmp + value; //calculate the new value of the counter after addition
	if (opt_yield)
	  sched_yield();
    } while(__sync_val_compare_and_swap(pointer, tmp, sum) != tmp); //swap new counter value into the pointer and 
  }                                                                 //finish if the new value is inserted
}

//thread function
void* threader(void* arg){
  int i = 0;
  //inrements and then decrements number of iterations
  for(i = 0; i < iterations; i++){
    add(arg, 1);
  }
  for(i = 0; i < iterations; i++){
    add(arg, -1);
  }
  pthread_exit(0);
}

int main(int argc, char** argv){
  int thread_num = 1;
  int y = 0;
  struct option opts[] = {
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"yield", no_argument, NULL, 'y'},
    {"sync", required_argument, NULL, 's'},
    {0, 0, 0, 0}
  };

  char optresult;
  //parse options
  while ((optresult = getopt_long(argc, argv, "", opts, NULL)) != -1){
    switch (optresult){
    case 's':
      sync_type = optarg[0];
      if ((sync_type !='m' && sync_type != 's' && sync_type != 'c') || (strlen(optarg) > 1)){
	fprintf(stderr, "Incorrect sync option. Correct usage is ./lab2_add [--threads=<number>] [--iterations=<number>] [--sync=<smc>]\n");
	exit(1);
      }
      break;
    case 'y':
      y = 1;
      opt_yield = 1;
      break;
    case 't':
      thread_num = atoi(optarg);
      break;
    case 'i':
      iterations = atoi(optarg);
      break;
    case 0:
      break;
    case '?':
    default:
      fprintf(stderr, "Correct usage is ./lab2_add [--threads=<number>] [--iterations=<number>] [--sync=<smc>]\n");
      exit(1);
    }
  }
  //intiaizlize the mutex lock if that condition is specified
  if (sync_type == 'm'){
    if (pthread_mutex_init(&lock, NULL) != 0){
      fprintf(stderr, "mutex init failed\n");
	exit(1);
    }
  }
  
  long long counter = 0;
  int operations = 2*iterations*thread_num;
  struct timespec start, end;
  pthread_t t[thread_num];
  int i = 0;
  int j = 0;

  //get the start time
  if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
    fprintf(stderr, "Error: clock gettime failed\n");
    exit(1);
  }

  //create each thread
  for (i = 0; i < thread_num; i++) {
    if (pthread_create(&t[i], NULL, threader, &counter)) {
      fprintf(stderr, "Error: thread creation error");
      exit(2);
    }
  }

  //wait for each thread to finish running
  for (j = 0; j < thread_num; j++)
    if (pthread_join(t[j], NULL)) {
      fprintf(stderr, "Error: thread joining error");
      exit(2);
    }

  //get the finish time
  if(clock_gettime(CLOCK_MONOTONIC, &end) == -1){
    fprintf(stderr, "Error: clock gettime failed\n");
    exit(1);
  }

  //calculate the run time and time per operation
  long long time = 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
  long long tpo = time / operations;
  
  //print out results depending on the specified options
  if (y){
    if (sync_type == 'm'){
      fprintf(stdout, "add-yield-m,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
    else if (sync_type == 's'){
      fprintf(stdout, "add-yield-s,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
    else if (sync_type == 'c'){
      fprintf(stdout, "add-yield-c,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
    else {
      fprintf(stdout, "add-yield-none,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
  }
  else{
    if (sync_type == 'm'){
      fprintf(stdout, "add-m,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
    else if (sync_type == 's'){
      fprintf(stdout, "add-s,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
    else if (sync_type == 'c'){
      fprintf(stdout, "add-c,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
    else {
      fprintf(stdout, "add-none,%d,%d,%d,%lld,%lld,%lld\n", thread_num, iterations, operations, time, tpo, counter);
    }
  }

  //clean up lock and exit
  if (sync_type == 'm'){
    pthread_mutex_destroy(&lock);
  }
  exit(0);
}
