/**
 * Scheduling Simulation
 *
 * Authors: Ryan Seys and Osazuwa Omigie
 *
 * Supports FCFS (First Come First Serve)
 * Supports SJF (Shortest Job First)
 * Supports SRTF (Shortest Remaining Time First)
 * Supports Round Robin Time Slicing
 * Supports I/O Operation Duration/Frequency
 *
 * Accepts a file where each line is a comma separated string.
 * e.g.

1,0,22,5,1,2
3,12,12,5,1,2
5,17,14,5,1,2
2,9,11,5,1,2
4,13,11,5,1,2

 *
 * The above sample file has 5 lines (5 processes), with each process having six
 * (6) values (pid, start time, total cpu time, io frequency (or zero [0]), io duration (or zero [0]), round robin time slice frequency)
 *
 * Three sample files for FCFS, SJF and SRTF are provided. They can be modified for your experimentation.
 * Each sample will be automatically run with their respective algorithms.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <assert.h>
#include <limits.h>

//definitions

#define INITIAL_TIME 0

//state codes
#define READY_STATE 1
#define RUNNING_STATE 2
#define WAITING_STATE 3
#define TERMINATED_STATE 4
#define NEW_STATE 5
#define INVALID_MOVE -1

//strings for output files
#define READY_STATE_STR "READY"
#define WAITING_STATE_STR "WAITING"
#define NEW_STATE_STR "NEW"
#define TERMINATED_STATE_STR "TERMINATED"
#define RUNNING_STATE_STR "RUNNING"
#define UNKNOWN_STATE_STR "UNKNOWN"

//move codes
#define NEW_TO_READY 1
#define RUNNING_TO_READY 6
#define WAITING_TO_READY 5
#define RUNNING_TO_TERMINATED 3
#define RUNNING_TO_WAITING 4
#define READY_TO_RUNNING 2

//sorting algorithms
#define FCFS_SORT 0
#define SJF_SORT 1
#define SRTF_SORT 2

//input and output files
#define FCFS_INPUT "fcfs.txt"
#define SJF_INPUT "sjf.txt"
#define SRTF_INPUT "srtf.txt"
#define FCFS_OUTPUT "fcfs_results.txt"
#define SJF_OUTPUT "sjf_results.txt"
#define SRTF_OUTPUT "srtf_results.txt"

/**
 * Using a double ended Queue
 * From http://developer.gnome.org/glib/2.34/glib-Double-ended-Queues.html
*/

/**
 * Represents a process to be stored in a Queue
 *
 * pid: process id
 * start: start time
 * total: total amount of cpu time
 * iofreq: how many seconds between each io operation
 * iodur: duration of io operations
 * remaining: remaining amount of cpu time to execute
 * last_start: last time the process was started
 * last_io_start: last time the process did io
 * rr: round robin frequency
 */
struct process {
    int pid;
    int start;
    int total;
    int iofreq;
    int iodur;
    int remaining;
    int last_start;
    int last_io_start;
    int rr;
};

typedef struct process Process;

/**
 * FCFS Sorting Algorithm: Compares 2 processes in a queue based on their time of arrival
 * @param  a    First process
 * @param  b    Second process
 * @param  data data (typically NULL in this case)
 * @return      a negative value if a < b, 0 if a = b, a positive value if a > b
 */
gint sort_fcfs(gconstpointer a, gconstpointer b, gpointer data) {
  Process * ap = (Process *) a;
  Process * bp = (Process *) b;
  return ap->start - bp->start; // sort so head is the least start time
}

/**
 * Shortest Job First: Compares 2 processes in a queue based on their execution time
 * @param  a    First process
 * @param  b    Second process
 * @param  data unused var
 * @return      a negative value if a < b, 0 if a = b, a positive value if a > b
 */
gint sort_sjf(gconstpointer a, gconstpointer b, gpointer data) {
  Process * ap = (Process *) a;
  Process * bp = (Process *) b;
  return ap->total - bp->total; // sort so head is least total first
}

/**
 * Shortest Remaining Time First: Compares 2 processes in a queue based on their remaining execution time
 * @param  a    First process
 * @param  b    Second process
 * @param  data unused var
 * @return      a negative value if a < b, 0 if a = b, a positive value if a > b
 */
gint sort_srtf(gconstpointer a, gconstpointer b, gpointer data) {
  Process * ap = (Process *) a;
  Process * bp = (Process *) b;
  return ap->remaining - bp->remaining; // sort so head is least remaining time first
}

gint (*fcfs_algorithm)(gconstpointer,gconstpointer,gpointer);
gint (*sjf_algorithm)(gconstpointer,gconstpointer,gpointer);
gint (*srtf_algorithm)(gconstpointer,gconstpointer,gpointer);

/**
 * Initializes a process with passed in paramaters
 * @param  pid    Process' PID
 * @param  start  start time
 * @param  total  Total execution time
 * @param  iofreq how oftem the process does I/O
 * @param  iodur  I/O duration time
 * @return        pointer to the initialized variable
 */
Process * process_new(int pid, int start, int total, int iofreq, int iodur, int rr) {
  Process *p = malloc(sizeof(Process));
  assert (p != NULL);
  p->pid = pid; //process id
  p->start = start; //start time
  p->total = total; //total amount of cpu time
  p->iofreq = iofreq; //how many seconds between each io operation
  p->iodur = iodur; //duration of io operations
  p->remaining = total; //remaining amount of cpu time to execute
  p->rr = rr; //round robin frequency
  return p;
}

char * get_state_string(int state) {
  switch(state) {
    case READY_STATE:
      return READY_STATE_STR;
    break;

    case RUNNING_STATE:
      return RUNNING_STATE_STR;
    break;

    case WAITING_STATE:
      return WAITING_STATE_STR;
    break;

    case TERMINATED_STATE:
      return TERMINATED_STATE_STR;
    break;

    case NEW_STATE:
      return NEW_STATE_STR;
    break;

    default:
      return UNKNOWN_STATE_STR;
    break;
  }
}

void write_update(const char * filename, int tot, int pid, int old, int new) {
  FILE *file;

  file = fopen(filename,"a+"); /* apend file (add text to a file or create a file if it does not exist.*/
  fprintf(file, "%d\t%d\t%s\t\t%s\n", tot, pid, get_state_string(old), get_state_string(new)); //writes
  fclose(file);
}

/**
 * Writes results to a file
 * @param filename name of file
 * @param val      what to write (append)
 */
void write_to_file(const char * filename, const char * val) {
  FILE *file;

  file = fopen(filename,"a+"); /* apend file (add text to a file or create a file if it does not exist.*/
  fprintf(file, "%s", val); //writes
  fclose(file); //done writing to file
}

/**
 * Creates a queue of processes from the text input data
 * @param  filename name of file
 * @return          queue of processes
 */
GQueue * parse_file(const char * filename) {
  GQueue * queue;
  FILE * fp;
  int pid, start, total, iofreq, iodur, rr;

  queue = g_queue_new();

  if((fp = fopen(filename, "r+")) == NULL) {
    printf("No such file\n");
    exit(1);
  }

  while(fscanf(fp,"%d,%d,%d,%d,%d,%d", &pid, &start, &total, &iofreq, &iodur, &rr) == 6) {

    iofreq = iofreq <= 0 ? INT_MAX : iofreq; // assume to be no I/O if given negative or 0 (happens only at max simulation time)
    iodur = iodur <= 0 ? INT_MAX : iodur; //assume to be no I/O if given negative or 0 (happens only at max simulation time)
    rr = rr <= 0 ? INT_MAX : rr; // assume to be no preemption if given negative or 0 (happens only at max simulation time)
    total = total < 0 ? 0 : total; //assume to be zero if given negative value
    start = start < 0 ? 0 : start; //assume to be zero if given negative value

    Process *p = process_new(pid, start, total, iofreq, iodur, rr);
    g_queue_push_head(queue, p);
  }

  if(feof(fp)) {
    printf("Finished processing %s\n", filename);
    return queue;
  }
  else {
    printf("Error reading! Invalid format!\n");
    return queue;
  }
}

/**
 * Print the size of the queue
 * @param q queue
 */
void print_size_of_queue(GQueue * q) {
  printf("%d\n", (int) g_queue_get_length(q));
}

/**
 * moves a process from the head of a non-empty queue to the tail of another queue
 * @param from SOURCE queue
 * @param to   DESTINATION queue
 */
void move_process(GQueue * from, GQueue * to) {
  assert(!g_queue_is_empty(from)); //from cannot be empty
  g_queue_push_tail(to, g_queue_pop_head(from));
}

/**
 * gets the start value of the process in a queue
 * @param  q the queue to do the operation on
 * @return   the start value of the process in a queue
 */
int get_head_start_val(GQueue * q) {
  return ((Process *) g_queue_peek_head(q))->start;
}

/**
 * Return the pid of the tail of the queue
 * @param  q the queue to do the operation on
 * @return   the pid of the tail of the queue
 */
int get_tail_pid(GQueue * q) {
  return ((Process *) g_queue_peek_tail(q))->pid;
}

/**
 * Get the IO duration of the tail of the queue
 * @param  q the queue to do the operation on
 * @return   the IO duration of the tail of the queue
 */
int get_tail_iodur(GQueue * q) {
  return ((Process *) g_queue_peek_tail(q))->iodur;
}

/**
 * Get the IO duration of the head of the queue
 * @param  q the queue to do the operation on
 * @return   the IO duration of the head of the queue
 */
int get_head_iodur(GQueue * q) {
  return ((Process *) g_queue_peek_head(q))->iodur;
}

/**
 * Get the IO frequency of the head of the queue
 * @param  q the queue to do the operation on
 * @return   the IO frequency of the head of the queue
 */
int get_head_iofreq_val(GQueue * q) {
  return ((Process *) g_queue_peek_head(q))->iofreq;
}

/**
 * Get the IO frequency of the tail of the queue
 * @param  q the queue to do the operation on
 * @return   the IO frequency of the tail of the queue
 */
int get_tail_iofreq_val(GQueue * q) {
  return ((Process *) g_queue_peek_tail(q))->iofreq;
}

/**
 * Get the next IO time of the head of the queue
 * @param  q            the queue
 * @param  current_time current time
 * @return              the next IO time of the head of the queue
 */
int get_next_io_time(GQueue * q, int current_time) {
  return get_head_last_start(q) + get_head_iofreq_val(q);
}

/**
 * Get the remaining cpu time of the tail of the queue
 * @param  q the queue to do the operation on
 * @return   the remaining cpu time of the tail of the queue
 */
int get_tail_remaining_time(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_tail(q);
  return proc->remaining;
}

/**
 * Get the remaining cpu time of the head of the queue
 * @param  q the queue to do the operation on
 * @return   the remaining cpu time of the head of the queue
 */
int get_head_remaining_time(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_head(q);
  return proc->remaining;
}

/**
 * Get the Round Robin frequency for the head of the queue
 * @param  q the queue
 * @return   the Round Robin frequency for the head of the queue
 */
int get_head_rr_freq(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_head(q);
  return proc->rr;
}

/**
 * Get the Round Robin frequency for the head of the queue
 * @param  q the queue
 * @return   the Round Robin frequency for the head of the queue
 */
int get_tail_rr_freq(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_tail(q);
  return proc->rr;
}

/**
 * Set the remaining time on the tail process of the queue
 * @param q   the queue
 * @param val the new remaining time value
 */
void set_tail_remaining_time(GQueue * q, int val) {
  Process * proc = (Process *) g_queue_peek_tail(q);
  proc->remaining = val;
}

/**
 * Set the remaining time on the head process of the queue
 * @param q   the queue
 * @param val the new remaining time value
 */
void set_head_last_start(GQueue * q, int val) {
  Process * proc = (Process *) g_queue_peek_head(q);
  proc->last_start = val;
}

/**
 * Get the last start time of the head process of the queue
 * @param  q the queue to do the operation on
 * @return   the value of the last start
 */
int get_head_last_start(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_head(q);
  return proc->last_start;
}

/**
 * Get the last start time of the tail process of the queue
 * @param  q the queue to do the operation on
 * @return   the value of the last start
 */
int get_tail_last_start(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_tail(q);
  return proc->last_start;
}

/**
 * Set the IO last start on the tail process of the queue
 * @param q   the queue
 * @param val the new IO last start time
 */
void set_tail_io_start(GQueue * q, int val) {
  Process * proc = (Process *) g_queue_peek_tail(q);
  proc->last_io_start = val;
}

/**
 * Get the IO last start on the tail process of the queue
 * @param q   the queue
 * @param val the new IO last start time
 */
int get_tail_last_io_start(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_tail(q);
  return proc->last_io_start;
}

/*
 * Get the IO last start on the head process of the queue
 * @param q   the queue
 * @param val the new IO last start time
 */
int get_head_last_io_start(GQueue * q) {
  Process * proc = (Process *) g_queue_peek_head(q);
  return proc->last_io_start;
}

/**
 * Moves a process from one state to another. The state transitions are determined by the 'move' paramater.
 * new/all --> ready; ready --> runninig; running --> terminated; running --> waiting; waiting --> ready; waiting --> ready; running --> ready.
 *
 * @param from
 * @param to
 * @param move
 * @param current_time
 * @param sort
 */
void execute_move(GQueue * from, GQueue * to, int move, int current_time, int sort) {
  const char * output_file;
  gboolean must_sort = FALSE;

  move_process(from, to);

  if(sort == FCFS_SORT) output_file = FCFS_OUTPUT;
  else if(sort == SJF_SORT) output_file = SJF_OUTPUT;
  else if(sort == SRTF_SORT) output_file = SRTF_OUTPUT;

  switch(move) {
    case NEW_TO_READY: // all --> ready
      must_sort = TRUE;
      write_update(output_file, current_time, get_tail_pid(to), NEW_STATE, READY_STATE);
      break;

    case READY_TO_RUNNING: // ready --> running
      set_head_last_start(to, current_time);
      write_update(output_file, current_time, get_tail_pid(to), READY_STATE, RUNNING_STATE);
      break;

    case RUNNING_TO_TERMINATED: // running --> terminated
      set_tail_remaining_time(to, 0);
      write_update(output_file, current_time, get_tail_pid(to), RUNNING_STATE, TERMINATED_STATE);
      break;

    case RUNNING_TO_WAITING: // running --> waiting
      set_tail_remaining_time(to, get_tail_remaining_time(to)-get_tail_iofreq_val(to));
      set_tail_io_start(to, current_time);
      write_update(output_file, current_time, get_tail_pid(to), RUNNING_STATE, WAITING_STATE);
      break;

    case WAITING_TO_READY: // waiting --> ready
      must_sort = TRUE;
      write_update(output_file, current_time, get_tail_pid(to), WAITING_STATE, READY_STATE);
      break;

    case RUNNING_TO_READY: // running --> ready
      must_sort = TRUE;
      set_tail_remaining_time(to, get_tail_remaining_time(to)-get_tail_rr_freq(to));
      write_update(output_file, current_time, get_tail_pid(to), RUNNING_STATE, READY_STATE);
      break;

    default:
      return;
      break;
  }

  if(must_sort) {
    if(sort == SJF_SORT) g_queue_sort(to, sjf_algorithm, NULL);
    else if(sort == SRTF_SORT) g_queue_sort(to, srtf_algorithm, NULL);
  }
}

/**
 * Determines which transitions to make and calls the execute_move() method
 * @param  all
 * @param  ready
 * @param  running
 * @param  waiting
 * @param  terminated
 * @param  current_time
 * @param  sort
 * @return
 */
int get_next_move(GQueue * all, GQueue * ready, GQueue * running, GQueue * waiting, GQueue * terminated, int current_time, int sort) {
  assert(current_time >= 0);
  int move = INVALID_MOVE;
  int all_to_ready = INT_MAX, ready_to_running = INT_MAX,running_to_terminated = INT_MAX,
    running_to_waiting = INT_MAX, waiting_to_ready = INT_MAX, running_to_ready = INT_MAX;

  GQueue * from;
  GQueue * to;

  /*-----
   The next move is determined based on the SOONEST of any of these times:
            - ArrivalTime of processes in the new state OR
            - I/O time of currently running process OR
            - End time of a process' I/O operation OR
            - Remaining time left for the process to complete execution
            - Next I/O time for the currently running process
   --------*/
  if(!g_queue_is_empty(all)) all_to_ready = get_head_start_val(all);
  if(!g_queue_is_empty(ready) && g_queue_is_empty(running)) ready_to_running = MAX(current_time,get_head_start_val(ready));
  if(!g_queue_is_empty(running)) running_to_waiting = get_next_io_time(running, current_time);
  if(!g_queue_is_empty(running)) running_to_terminated = get_head_remaining_time(running) + get_head_last_start(running);
  if(!g_queue_is_empty(waiting)) waiting_to_ready = get_head_last_io_start(waiting) + get_head_iodur(waiting);
  if(!g_queue_is_empty(running)) running_to_ready = get_head_last_start(running) + get_head_rr_freq(running);

  //sanitize any addition overflows
  all_to_ready = all_to_ready < 0 ? INT_MAX : all_to_ready;
  ready_to_running = ready_to_running < 0 ? INT_MAX : ready_to_running;
  running_to_waiting = running_to_waiting < 0 ? INT_MAX : running_to_waiting;
  running_to_terminated = running_to_terminated < 0 ? INT_MAX : running_to_terminated;
  waiting_to_ready = waiting_to_ready < 0 ? INT_MAX : waiting_to_ready;
  running_to_ready = running_to_ready < 0 ? INT_MAX : running_to_ready;

  int min = MIN(MIN(MIN(MIN(MIN(all_to_ready, ready_to_running), running_to_terminated), running_to_waiting), waiting_to_ready), running_to_ready);

  if(min == INT_MAX) return move;
  else if(min == waiting_to_ready) {
    move = WAITING_TO_READY;
    from = waiting;
    to = ready;
  }
  else if(min == all_to_ready) {
    move = NEW_TO_READY;
    from = all;
    to = ready;
  }

  else if(min == running_to_ready) {
    move = RUNNING_TO_READY;
    from = running;
    to = ready;
  }
  else if(min == running_to_waiting) {
    move = RUNNING_TO_WAITING;
    from = running;
    to = waiting;
  }
  else if(min == running_to_terminated) {
    move = RUNNING_TO_TERMINATED;
    from = running;
    to = terminated;
  }
  else if(min == ready_to_running) {
    move = READY_TO_RUNNING;
    from = ready;
    to = running;
  }
  else return move;
  /*--------------------------------------------------*/
  //ready to running is being checked before waiting to ready and that is wrong!

  current_time = min;
  execute_move(from, to, move, current_time, sort); //execute the move
  return current_time;
}

/**
 * Reallocate all the processes from these queues to the system
 *
 * @param all        the all/new queue
 * @param ready      the ready queue
 * @param running    the running queue
 * @param waiting    the waiting
 * @param terminated the terminated queue
 */
void realloc_q_procs(GQueue * all, GQueue * ready, GQueue * running, GQueue * waiting, GQueue * terminated) {
  while(!g_queue_is_empty(all)) {
    free(g_queue_pop_head(all));
  }
  while(!g_queue_is_empty(ready)) {
    free(g_queue_pop_head(ready));
  }
  while(!g_queue_is_empty(running)) {
    free(g_queue_pop_head(running));
  }
  while(!g_queue_is_empty(waiting)) {
    free(g_queue_pop_head(waiting));
  }
  while(!g_queue_is_empty(terminated)) {
    free(g_queue_pop_head(terminated));
  }
}

/**
 * Reallocate all the q to the system by freeing all processes in the queues
 * and then by freeing the queues themselves
 */
void realloc_q(GQueue * all, GQueue * ready, GQueue * running, GQueue * waiting, GQueue * terminated) {
  realloc_q_procs(all, ready, running, waiting, terminated);
  g_queue_free(all);
  g_queue_free(ready);
  g_queue_free(running);
  g_queue_free(waiting);
  g_queue_free(terminated);
}

/**
 * The main method is the driver for the different input files. Here we have specified
 * three different default files for testing FCFS algorithm, SJF algorithm and SRTF algorithm
 * @return [description]
 */
int main() {
  int t = INITIAL_TIME;
  /**/
  sjf_algorithm = &sort_sjf;
  fcfs_algorithm = &sort_fcfs;
  srtf_algorithm = &sort_srtf;

  GQueue * all;
  //Queues below named after the different states of the processes
  GQueue * ready = g_queue_new();
  GQueue * running = g_queue_new();
  GQueue * waiting = g_queue_new();
  GQueue * terminated = g_queue_new();

  // First Come First Serve
  write_to_file(FCFS_OUTPUT, "--- FIRST COME FIRST SERVE SCHEDULING SIMULATION ---\n");
  write_to_file(FCFS_OUTPUT, "time\tpid\told state\tnew state\n");

  all = parse_file(FCFS_INPUT); //populates the 'all' queue with the text Ginput data
  g_queue_sort(all, fcfs_algorithm, NULL);
  while((t = get_next_move(all, ready, running, waiting, terminated, t, FCFS_SORT)) != INVALID_MOVE); //
  printf("FCFS simulation trace written to: %s\n\n", FCFS_OUTPUT);
  //reset
  realloc_q_procs(all, ready, running, waiting, terminated);
  t = INITIAL_TIME;

  // Shortest Job First
  write_to_file(SJF_OUTPUT, "--- SHORTEST JOB FIRST SCHEDULING SIMULATION ---\n");
  write_to_file(SJF_OUTPUT, "time\tpid\told state\tnew state\n");

  all = parse_file(SJF_INPUT);
  g_queue_sort(all, fcfs_algorithm, NULL); //sort in earliest first always
  while((t = get_next_move(all, ready, running, waiting, terminated, t, SJF_SORT)) != INVALID_MOVE);
  printf("SJF simulation trace written to: %s\n\n", SJF_OUTPUT);
  //reset
  realloc_q_procs(all, ready, running, waiting, terminated);
  t = INITIAL_TIME;

  // Shortest Remaining Time First
  write_to_file(SRTF_OUTPUT, "--- SHORTEST REMAINING TIME FIRST SCHEDULING SIMULATION ---\n");
  write_to_file(SRTF_OUTPUT, "time\tpid\told state\tnew state\n");

  all = parse_file(SRTF_INPUT);
  g_queue_sort(all, fcfs_algorithm, NULL); //sort in earliest first always
  while((t = get_next_move(all, ready, running, waiting, terminated, t, SRTF_SORT)) != INVALID_MOVE);
  printf("SRTF simulation trace written to: %s\n", SRTF_OUTPUT);

  realloc_q(all, ready, running, waiting, terminated);
  return 0;
}

// Created by Ryan Seys and Osazuwa Omigie
