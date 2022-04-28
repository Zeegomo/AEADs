
#include "pmsis.h"
#include <bsp/bsp.h>


#define HOTTING 2
#define REPEAT  5
#define STACK_SIZE      1024
#define LEN             1

PI_L1 char data[LEN];
PI_L1 char data2[LEN];
PI_L1 char key[32];

void encrypt(char *data, size_t len, char *key, size_t num_cores);

void encrypt_serial(char *data, size_t len, char *key);
#define INIT_STATS()  

#define ENTER_STATS_LOOP()  \
    unsigned long _cycles = 0; \
    unsigned long _instr = 0; \
    unsigned long _active = 0; \
    unsigned long _ldext = 0; \
    unsigned long _tcdmcont = 0; \
    unsigned long _ldstall = 0; \
    unsigned long _imiss = 0; \
    for(int _k=0; _k<HOTTING+REPEAT; _k++) { \
      pi_perf_conf((1<<PI_PERF_CYCLES) | (1<<PI_PERF_INSTR) | (1<<PI_PERF_ACTIVE_CYCLES) | (1<<PI_PERF_LD_EXT) | (1<<PI_PERF_TCDM_CONT) | (1<<PI_PERF_LD_STALL) | (1<<PI_PERF_IMISS) );


#define START_STATS()  \
    pi_perf_reset(); \
    pi_perf_start();

#define STOP_STATS() \
     pi_perf_stop(); \
     if (_k >= HOTTING) \
      { \
        _cycles   += pi_perf_read (PI_PERF_CYCLES); \
        _instr    += pi_perf_read (PI_PERF_INSTR); \
    	_active   += pi_perf_read (PI_PERF_ACTIVE_CYCLES); \
        _ldext    += pi_perf_read (PI_PERF_LD_EXT); \
    	_tcdmcont += pi_perf_read (PI_PERF_TCDM_CONT); \
    	_ldstall  += pi_perf_read (PI_PERF_LD_STALL); \
        _imiss    += pi_perf_read (PI_PERF_IMISS); \
      }

#define EXIT_STATS_LOOP()  \
    } \
    printf("[%d] total cycles = %lu\n", pi_core_id(), _cycles/REPEAT); \
    printf("[%d] instructions = %lu\n", pi_core_id(), _instr/REPEAT); \
    printf("[%d] active cycles = %lu\n", pi_core_id(), _active/REPEAT); \
    printf("[%d] external loads (L2+synch) = %lu\n", pi_core_id(), _ldext/REPEAT); \
    printf("[%d] TCDM cont = %lu\n", pi_core_id(), _tcdmcont/REPEAT); \
    printf("[%d] LD stalls = %lu\n", pi_core_id(), _ldstall/REPEAT); \
    printf("[%d] I$ misses = %lu\n", pi_core_id(), _imiss/REPEAT);

// Cluster entry point
static void cluster_entry(void *arg)
{
 // init performance counters
  INIT_STATS();

  // executing the code multiple times to perform average statistics
  ENTER_STATS_LOOP();
  START_STATS();
  encrypt(data, LEN, key, NUM_CORES);
  STOP_STATS();

  // end of the performance statistics loop
  EXIT_STATS_LOOP();
}

void pi_cl_team_fork_tmp(int nb_cores, void (*entry)(void *), void *arg)
{
    pi_cl_team_fork(nb_cores, entry, arg);
}
static void cluster_entry2(void *arg)
{
 // init performance counters
  INIT_STATS();

  // executing the code multiple times to perform average statistics
  ENTER_STATS_LOOP();
  START_STATS();
  encrypt_serial(data2, LEN, key);
  STOP_STATS();

  // end of the performance statistics loop
  EXIT_STATS_LOOP();
}

static void __attribute__((optimize("O0"))) cluster_entry3(void *arg)
{
  data2[0] = 123;
  int a = data[2];
}


int main()
{
  struct pi_device cluster_dev = {0};
  struct pi_cluster_conf conf;
  struct pi_cluster_task cluster_task = {0};
  //pi_l2_malloc_align(4, 4);

  // task parameters allocation
  pi_cluster_task(&cluster_task, cluster_entry3, NULL);

  // [OPTIONAL] specify the stack size for the task
  cluster_task.stack_size = STACK_SIZE;
  cluster_task.slave_stack_size = STACK_SIZE;

  // open the cluster
  pi_cluster_conf_init(&conf);
  pi_open_from_conf(&cluster_dev, &conf);
  if (pi_cluster_open(&cluster_dev))
  {
    printf("ERROR: Cluster not working\n");
    return -1;
  }
  for(int i = 0; i < 32; i++)
    key[i] = 0;

  for(int i = 0; i < LEN; i++){
      data[i] = 0;
      data2[i] = 0;
  }

  printf("start: %c %c\n", data[0], data[LEN-1]);

  // send a task to the cluster
  pi_cluster_send_task_to_cl(&cluster_dev, &cluster_task);
  printf("encrypt: %c %c\n", data[0], data[LEN-1]);
  
  // pi_cluster_task(&cluster_task, cluster_entry3, NULL);
  // pi_cluster_send_task_to_cl(&cluster_dev, &cluster_task);
  //encrypt_serial(data2, LEN, key);


  for(int i = 0; i < LEN; i++){
      if (data[i] !=data2[i]) {
          printf("WRONG %d %d %d\n", i, data[i], data2[i]);
          break;
      }
  }

  // closing the cluster
  pi_cluster_close(&cluster_dev);
    


  return 0;
 
}