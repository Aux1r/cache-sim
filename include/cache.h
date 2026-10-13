/*
 ********************************************************
  Please do not edit anything in this file.
 ********************************************************
*/

#ifndef CACHE_H_
#define CACHE_H_

#include "common.h"
#include <stdbool.h>

// Cache statistics counters.
extern uint32_t L1_cache_total_accesses;
extern uint32_t L1_cache_hits;
extern uint32_t L1_cache_misses;
extern uint32_t L1_cache_read_accesses;
extern uint32_t L1_cache_read_hits;
extern uint32_t L1_cache_write_accesses;
extern uint32_t L1_cache_write_hits;
extern uint32_t L2_cache_total_accesses;
extern uint32_t L2_cache_hits;
extern uint32_t L2_cache_misses;
extern uint32_t L2_cache_read_accesses;
extern uint32_t L2_cache_read_hits;
extern uint32_t L2_cache_write_accesses;
extern uint32_t L2_cache_write_hits;
extern uint32_t memory_total_accesses;
extern uint32_t memory_read_accesses;
extern uint32_t memory_write_accesses;

// Input parameters to control the cache.
extern uint32_t cache_level;
extern uint32_t L1_cache_size;
extern uint32_t L1_cache_associativity;
extern uint32_t L1_cache_block_size;
extern uint32_t L2_cache_size;
extern uint32_t L2_cache_associativity;
extern uint32_t L2_cache_block_size;

void initialize_cache(void);
void free_cache(void);
void print_cache_statistics(void);
int check_cache_parameters_valid(void);

op_result_t read_from_cache(uint32_t pa);
op_result_t write_to_cache(uint32_t pa);

int process_arg_S(int opt, char *optarg);
int process_arg_A(int opt, char *optarg);
int process_arg_B(int opt, char *optarg);
int process_arg_L(int opt, char *optarg);
int process_arg_P(int opt, char *optarg);
void handle_cache_verbose(memory_access_entry_t entry, op_result_t ret);

typedef struct {
  bool valid;
  uint32_t tag;
  bool dirty;
} block_t;

extern block_t ***cache;

#endif /* CACHE_H_ */
