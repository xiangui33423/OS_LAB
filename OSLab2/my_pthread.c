// File:	my_pthread.c
// Author:	Yujie REN, Jieming Yin
// Date:	April 2025

#include "my_pthread_t.h"
#include <stdlib.h>
#include <sys/ucontext.h>
#include <ucontext.h>

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
#define MAX_THREAD_NUM 128
#define STACK_SIZE 64*1024	

static tcb thread_pool[MAX_THREAD_NUM]; // thread pool
static my_pthread_t thread_count = 0;
static my_pthread_t current_thread = 0;
static ucontext_t scheduler_context;

// Ready queue for PSJF
static Queue* ready_queue;
static void enqueue(Queue *queue, tcb *thread)
{
	Node *new_node = (Node *)malloc(sizeof(Node));
	new_node->thread = thread;
	new_node->next = NULL;
	if (queue->roar == NULL) {
		queue->front = queue->roar = new_node;
	} else {
		queue->roar->next = new_node;
		queue->roar = new_node;
	}
	return;
}

static tcb* get_thread_by_id(my_pthread_t t_id)
{
	if (t_id < MAX_THREAD_NUM && thread_pool[t_id].threadId == t_id) {
		return &thread_pool[t_id];
	}
	return NULL;
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {
	// Create Thread Control Block
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// After everything is all set, push this thread into run queue				
	// YOUR CODE HERE
	my_pthread_t tcb_id = thread_count++;
	tcb *new_thread = &thread_pool[tcb_id];
	new_thread->threadId = tcb_id;
	new_thread->status = NOT_STARTED;
	new_thread->policy = POLICY_RR;

	getcontext(&(new_thread->context));
	new_thread->stack = malloc(STACK_SIZE);
	if (new_thread->stack == NULL) {
		return -1;
	}

	new_thread->context.uc_link = 0;
	new_thread->context.uc_stack.ss_sp = new_thread->stack;
	new_thread->context.uc_stack.ss_size = STACK_SIZE;
	new_thread->context.uc_stack.ss_flags = 0;
	if (new_thread->context.uc_stack.ss_sp == 0) {
		perror( "malloc: Could not allocate stack" );
        exit( 1 );
	}

	makecontext(&(new_thread->context), (void (*)(void)) function, 0);
	new_thread->status = READY;
	#ifndef MLFQ
    enqueue(ready_queue, new_thread);
    #else
    enqueue(mlfq->queues[0], new_thread);  // Start at highest priority queue
    #endif
    
	*thread = tcb_id;
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// Switch from thread context to scheduler context
	thread_pool[current_thread].status = READY;

	 // Add current thread back to ready queue
	 #ifndef MLFQ
	 enqueue(ready_queue, &thread_pool[current_thread]);
	 #else
	 // In MLFQ, add to appropriate level based on priority
	 enqueue(mlfq->queues[thread_pool[current_thread].priority], &thread_pool[current_thread]);
	 #endif

	swapcontext(&(thread_pool[current_thread].context), &scheduler_context);
	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	thread_pool[current_thread].status = FINISHED;

	thread_pool[current_thread].return_value = value_ptr;

	if (thread_pool[current_thread].waiting_thread != NULL) {
		thread_pool[current_thread].waiting_thread->status = READY;
		enqueue(ready_queue, thread_pool[current_thread].waiting_thread);
	}

	free(thread_pool[current_thread].stack);
	thread_pool[current_thread].stack = NULL;

	setcontext(&scheduler_context);
	// YOUR CODE HERE
};


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	// Waiting for a specific thread to terminate
	// Once this thread finishes,
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex
	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// Use the built-in test-and-set atomic function to test the mutex
	// If mutex is acquired successfuly, enter critical section
	// If acquiring mutex fails, push current thread into block list 
	// and context switch to scheduler 

	// YOUR CODE HERE
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in my_pthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else 
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

