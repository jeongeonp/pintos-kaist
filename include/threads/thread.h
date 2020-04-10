#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */

	/* LAB 1: Alarm Clock */
	int64_t wakeup_time;		/* LAB 1: Waking up time in ticks, refreshes every time */
	struct list_elem wait_elem;	/* LAB 1: List element for the wait queue */
	
	/* LAB 1: Priority Donation */
	int actual_priority;            /* LAB 1: The initial priority */
	struct lock *waiting_lock;      /* LAB 1: The lock that the thread is waiting on */
	struct list donations;          /* LAB 1: List of threads that have donated priority */
	struct list_elem don_elem;      /* LAB 1: List element of donations list */
	

	/*LAB 1: Advanced Scheduling */
	int nice;			/* LAB 1: Niceness */
	int recent_cpu;			/* LAB 1: CPU time taken recently */
	struct list_elem all_elem;	/* LAB 1: All list elem */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (int64_t tick);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

/* LAB 1: Alarm Clock */
void sleep_thread (struct thread *threads, int64_t ticks, int64_t current_time); /* LAB 1: puts the thread to sleep list */
void wake_thread (int64_t ticks); /* LAB 1: Wakes up the thread and puts in into ready queue */

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

/* LAB 1: Priority Scheduling and Donation */
void compare_top_priority (void); /* LAB 1: Checks with the top thread to yield */
bool compare_priority (struct list_elem *a, struct list_elem *b, void *aux UNUSED); /*LAB 1: Compares priority of threads */
void thread_preemption (void); /*LAB 1: Thread Preemption */

void donate_priority (void); /* LAB 1: Donates priority */
void erase_from_donations (struct lock *lock); /* LAB 1: Removes the thread waiting for the lock from donations list */
void redo_priority (void); /* LAB 1: Recalculates the priority after priority donation */

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/* LAB 1: Advanced Scheduling */
void scheduler_priority (struct thread *t);
void scheduler_recent_cpu (struct thread *t);
void scheduler_load_avg (void);
void scheduler_increment (void);
void scheduler_recalculate (void);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
