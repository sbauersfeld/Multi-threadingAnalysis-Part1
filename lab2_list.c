
#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

//race conditions may result in segementation faults, this function logs them
void signal_handler(int signo){
  if (signo == SIGSEGV){
    fprintf(stderr,"Received segmentation fault. Error message is: %s\n", strerror(errno));
    exit(2);
  }
}

//global variables shared by all threads
int flag = 0;
int iterations = 1;
SortedListElement_t *array;
int opt_yield = 0;
char sync_type = 'a';
SortedList_t head;
pthread_mutex_t lock;
int capacity;

//the function run by each thread
void* threader(void* arg){
  //each thread is passed an integer id
  int* s = (int*) arg;
  int start = *s;
  
  //begin processing the element array at a location determined by thread id 
  start = start * iterations;
  int end = start + iterations;
  int i = 0;

  //insert all elements into a single list
  for (i = start; i < end; i++){
    if (sync_type == 'm'){
      pthread_mutex_lock(&lock);
    }
    else if (sync_type == 's'){
      while(__sync_lock_test_and_set(&flag, 1));
    }
    
    SortedList_insert(&head, &array[i]);

    if (sync_type == 'm'){
      pthread_mutex_unlock(&lock);
    }
    else if (sync_type == 's'){
      __sync_lock_release(&flag);
    }
  }

  //this section finds the length of the list
  if (sync_type == 'm'){
    pthread_mutex_lock(&lock);
  }
  else if (sync_type == 's'){
    while(__sync_lock_test_and_set(&flag, 1));
  }
  
  int r = SortedList_length(&head);

  if (sync_type == 'm'){
    pthread_mutex_unlock(&lock);
  }
  else if (sync_type == 's'){
    __sync_lock_release(&flag);
  }

  //log an error if the list is malformed
  if (r == -1){
    fprintf(stderr, "Error finding length of list.");
    exit(2);
  }

  //this section looks up each key and deletes it
  SortedListElement_t *ptr = NULL;
  for (i = start; i < end; i++){
    if (sync_type == 'm'){
      pthread_mutex_lock(&lock);
    }
    else if (sync_type == 's'){
      while(__sync_lock_test_and_set(&flag, 1));
    }
    if((ptr = SortedList_lookup(&head, array[i].key)) == NULL){
      fprintf(stderr, "Error looking up item in the list.");
      exit(2);
    }
    if(SortedList_delete(ptr) == 1){
      fprintf(stderr, "Error deleting item in the list.");
      exit(2);
    }
    if (sync_type == 'm'){
      pthread_mutex_unlock(&lock);
    }
    else if (sync_type == 's'){
      __sync_lock_release(&flag);
    }
  }
  return NULL;
}

int main(int argc, char** argv){
  int thread_num = 1;
  char* yops = "none";
  struct option opts[] = {
    {"threads", required_argument, NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"yield", required_argument, NULL, 'y'},
    {"sync", required_argument, NULL, 's'},
    {0, 0, 0, 0}
  };
  int a = 0;
  char optresult;
  int length;
  int iopt = 0;
  int dopt = 0;
  int lopt = 0;

  //parses the options
  while ((optresult = getopt_long(argc, argv, "", opts, NULL)) != -1){
    switch (optresult){
    case 's':
      sync_type = optarg[0];
      if ((sync_type != 's' && sync_type != 'm') || strlen(optarg) > 1) {
	fprintf(stderr, "Correct usage is ./lab2_list [--threads=<number>] [--iterations=<number>] [--sync=<sm>] [--yield=<idl>]\n");
	exit(1);
      }
      break;
    case 'y':
      length = strlen(optarg);
      for (a = 0; a < length; a++){
	if (optarg[a] == 'i'){
	  opt_yield |= 0x1;
	  iopt = 1;
	}
	else if (optarg[a] == 'd'){
	  opt_yield |= 0x2;
	  dopt = 1;
	}
	else if (optarg[a] == 'l'){
	  opt_yield |= 0x4;
	  lopt = 1;
	}
	else {
	  fprintf(stderr, "Correct usage is ./lab2_list [--threads=<number>] [--iterations=<number>] [--sync=<sm>] [--yield=<idl>]\n");
	  exit(1);
	}
      }
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
      fprintf(stderr, "Correct usage is ./lab2_add [--threads=<number>] [--iterations=<number>] [--sync=<sm>] [--yield=<idl>]\n");
      exit(1);
    }
  }

  //changes string yops so that if reflects the specified options
  if (iopt && dopt && lopt){
    yops = "idl";
  }
  else if(iopt && dopt){
    yops = "id";
  }
  else if(iopt && lopt){
    yops = "il";
  }
  else if(lopt && dopt){
    yops = "dl";
  }
  else if(iopt){
    yops = "i";
  }
  else if(dopt){
    yops = "d";
  }
  else if(lopt){
    yops = "l";
  }

  //allocates data to hold the element array
  if (sync_type == 'm'){
    pthread_mutex_init(&lock, NULL);
  }
  srand(time(NULL));
  pthread_t threads[thread_num];
  int threadId[thread_num];
  capacity = thread_num * iterations;
  if((array = (SortedListElement_t*) malloc(sizeof(SortedListElement_t)*capacity)) == NULL ){
    fprintf(stderr, "Error: malloc failed\n");
    exit(2);
  }
  a = 0;

  //creates a random key for each element in the array
  for (a = 0; a < capacity; a++){
      int l = (rand() % 10) + 1;
      char *randkey = (char*) malloc((l+1)*sizeof(char));
      if (randkey == NULL){
	fprintf(stderr, "Error: malloc failed\n");
	exit(2);
      }
      int j = 0;
      for (j = 0; j < l; j++){
	randkey[j] = 33 + (rand() % 90);
      }
      randkey[l] = '\0';
      array[a].key = randkey;
  }
  
  //register signal handler and initialize the head linked list pointer
  signal(SIGSEGV, signal_handler);
  head.key = NULL;
  head.next = &head;
  head.prev = &head;

  a = 0;

  //get the start time
  struct timespec start, end;
  if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
    fprintf(stderr,"Clock gettime failure. Error message is: %s\n", strerror(errno));
    exit(1);
  }

  //create all the threads
  for (a = 0; a < thread_num; a++){
    threadId[a] = a;
    if (pthread_create(&threads[a], NULL, threader, &threadId[a])){
      fprintf(stderr, "Thread creation failure. Error message is: %s\n", strerror(errno));
      exit(2);
    }
  }

  //wait for al the threads to finish running
  a = 0;
  for (a = 0; a < thread_num; a++){
    if (pthread_join(threads[a], NULL)){
      fprintf(stderr, "Thread join failure. Error message is: %s\n", strerror(errno));
      exit(2);
    }
  }

  //get the finish time
  if(clock_gettime(CLOCK_MONOTONIC, &end) == -1){
    fprintf(stderr,"Clock gettime failure. Error message is: %s\n", strerror(errno));
    exit(1);
  }

  //ensure that the final length is zero
  int finalLength = SortedList_length(&head);
  if (finalLength != 0){
    fprintf(stderr, "Error: Final length is not zero.");
    free(array);
    pthread_mutex_destroy(&lock);
    exit(2);
  }

  //calculate the running time and time per operation
  int totalop = 3*iterations*thread_num;
  long long time = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
  long long tpo = time/totalop;

  //print results based on the specified options
  switch(sync_type){
  case 'a':
    fprintf(stdout, "list-%s-none,%d,%d,1,%d,%lld,%lld\n",yops,thread_num,iterations,totalop,time,tpo);
    break;
  case 's':
    fprintf(stdout, "list-%s-s,%d,%d,1,%d,%lld,%lld\n",yops,thread_num,iterations,totalop,time,tpo);
    break;
  case 'm':
    fprintf(stdout, "list-%s-m,%d,%d,1,%d,%lld,%lld\n",yops,thread_num,iterations,totalop,time,tpo);
    break;
  }

  //free allocated memory
  free(array);
  if (sync_type == 'm'){
    pthread_mutex_destroy(&lock);
  }
  exit(0);
}
