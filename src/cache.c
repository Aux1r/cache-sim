#include "cache.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Cache statistics counters.
uint32_t L1_cache_total_accesses;
uint32_t L1_cache_hits;
uint32_t L1_cache_misses;
uint32_t L1_cache_read_accesses;
uint32_t L1_cache_read_hits;
uint32_t L1_cache_write_accesses;
uint32_t L1_cache_write_hits;
uint32_t L2_cache_total_accesses;
uint32_t L2_cache_hits;
uint32_t L2_cache_misses;
uint32_t L2_cache_read_accesses;
uint32_t L2_cache_read_hits;
uint32_t L2_cache_write_accesses;
uint32_t L2_cache_write_hits;
uint32_t memory_total_accesses;
uint32_t memory_read_accesses;
uint32_t memory_write_accesses;

// Input parameters to control the cache.
uint32_t cache_level;
uint32_t L1_cache_size;
uint32_t L1_cache_associativity;
uint32_t L1_cache_block_size;
uint32_t L2_cache_size;
uint32_t L2_cache_associativity;
uint32_t L2_cache_block_size;

block_t ***cache;

/*
 * Perform a read from the memory for a particular address.
 * Since this is a cache-simulation, memory is not involved.
 * No need to implement the logic for this function.
 * Call this function when cache needs to read from memory.
 */
int read_from_memory(uint32_t pa) { return 0; }

/*
 * Perform a write from the cache to memory for a particular address.
 * Since this is a cache-simulation, memory is not involved.
 * No need to implement the logic for this function.
 * Call this function when cache needs to write to memory.
 */
int write_to_memory(uint32_t pa) { return 0; }

/*
 * Perform a read from the L2_cache for a particular address.
 * Again, no need to implement the logic for this function.
 * Call this function when the L1 cache needs to read from the L2 cache.
 */
int read_from_L2_cache(uint32_t pa) { return 0; }

/*
 * Perform a write from the L2 cache for a particular address.
 * Again, no need to implement the logic for this function.
 * Call this function when the L1 cache needs to write to the L2 cache.
 */
int write_to_L2_cache(uint32_t pa) { return 0; }

/*
 *********************************************************
  Please edit the below functions.
  You are allowed to add more functions that may help you
  with your implementation. However, please do not add
  them in any file. All your new code should be in cache.c
  file below this line.
 *********************************************************
*/

char* cache_prefetching_policy = NULL;

int calc_set_num(int size, int assoc, int block_size) {
  return size / (block_size * assoc); // calculate number of sets in cache
}

void mem_read() { // just so I don't have to keep copy & pasting 2 lines each time...
  memory_read_accesses += 1;
  memory_total_accesses += 1;
}

void mem_write() {
  memory_write_accesses += 1;
  memory_total_accesses += 1;
}

void L1_write() {
  L1_cache_write_accesses += 1;
  L1_cache_total_accesses += 1;
}

void L1_read() {
  L1_cache_read_accesses += 1;
  L1_cache_total_accesses += 1;
}

void L2_read() {
  L2_cache_read_accesses += 1;
  L2_cache_total_accesses += 1;
}

void L2_write() {
  L2_cache_write_accesses += 1;
  L2_cache_total_accesses += 1;
}

uint32_t calc_index(uint32_t pa, int cache_size) {
  // Tried doing this with shifts at first but had a weird overflow(?) error, do bit masking:
  // ok some shifting required, C is cursed
  int set_num = calc_set_num(cache_size, L1_cache_associativity, L1_cache_block_size);
  int index_b_num = log2(set_num);
  int offset_b_num = log2(L1_cache_block_size);
  uint32_t mask = ((pow(2, (index_b_num + offset_b_num))) - 1);
  uint32_t result = ((pa & mask) >> offset_b_num);
  return result;
}

uint32_t calc_tag(uint32_t pa, int cache_size) {
  int set_num = calc_set_num(cache_size, L1_cache_associativity, L1_cache_block_size);
  int offset_b_num = log2(L1_cache_block_size);
  int index_b_num = log2(set_num);
  return pa >> (offset_b_num + index_b_num); // just shift right by idx + offset to get tag
}

uint32_t convert_L1_tag_L2(uint32_t tag_L1, uint32_t index_L1, int L2_set_num) {
  int L1_set_num = calc_set_num(L1_cache_size, L1_cache_associativity, L1_cache_block_size);
  int offset_b_num_L1 = log2(L1_cache_block_size);
  int index_b_num_L1 = log2(L1_set_num);

  uint32_t original_pa = (tag_L1 << (offset_b_num_L1 + index_b_num_L1));
  original_pa = (original_pa + (index_L1 << (offset_b_num_L1)));

  int offset_b_num_L2 = log2(L1_cache_block_size);
  int index_b_num_L2 = log2(L2_set_num);

  uint32_t tag_L2 = (original_pa >> (index_b_num_L2 + offset_b_num_L2));
  return tag_L2;
}

uint32_t convert_L1_index_L2(uint32_t tag_L1, uint32_t index_L1, int L2_set_num) {
  int L1_set_num = calc_set_num(L1_cache_size, L1_cache_associativity, L1_cache_block_size);
  int offset_b_num_L1 = log2(L1_cache_block_size);
  int index_b_num_L1 = log2(L1_set_num);

  uint32_t original_pa = (tag_L1 << (offset_b_num_L1 + index_b_num_L1));
  original_pa = (original_pa + (index_L1 << (offset_b_num_L1)));

  int offset_b_num_L2 = log2(L1_cache_block_size);
  int index_b_num_L2 = log2(L2_set_num);

  uint32_t mask = ((pow(2, (index_b_num_L2 + offset_b_num_L2))) - 1);
  uint32_t result = ((original_pa & mask) >> offset_b_num_L2);
  return result;
}

void swap(int level, int set, int idx_1, int idx_2) {
  uint32_t temp_tag = cache[level][set][idx_1].tag;
  bool temp_valid = cache[level][set][idx_1].valid;
  bool temp_dirty = cache[level][set][idx_1].dirty;

  cache[level][set][idx_1].tag = cache[level][set][idx_2].tag;
  cache[level][set][idx_1].valid = cache[level][set][idx_2].valid;
  cache[level][set][idx_1].dirty = cache[level][set][idx_2].dirty;

  cache[level][set][idx_2].tag = temp_tag;
  cache[level][set][idx_2].valid = temp_valid;
  cache[level][set][idx_2].dirty = temp_dirty;

}

void LRUify_set(int level, int set, int set_idx) { // Call every time we need to update positions in set according to LRU
  for (int i = set_idx; i < L1_cache_associativity-1; i++) { // Swap enough times to get LRU in left-most index, MRU in right-most
    swap(level, set, i, i+1);
  }
}

void init_params() { // Init Params in check_valid_params
  // uint32_t defaults = 0, so check for 0 and then initialise
  if (!L1_cache_size) { // S
    L1_cache_size = 4096;
  }
  if (!L1_cache_block_size) { // B
    L1_cache_block_size = 4;
  }
  if (!L1_cache_associativity) { // A
    L1_cache_associativity = 1;
  }
  if (!cache_level) { // L
    cache_level = 1;
  }
  if (cache_prefetching_policy == NULL) { // P
    cache_prefetching_policy = "none";
  }
}

bool check_pow_2(int arg) {
  if (!(floor(log2(arg)) == log2(arg))) {
    return false;
  }
  return true;
}

// Some helper functions
void L2_evict(uint32_t L2_index, uint32_t L2_tag, uint32_t pa) {
  if (cache[1][L2_index][0].dirty) {
    mem_write();
    write_to_memory(pa);
  }
  cache[1][L2_index][0].valid = true;
  cache[1][L2_index][0].dirty = false;
  cache[1][L2_index][0].tag = L2_tag;
  LRUify_set(1, L2_index, 0);
}

void L1_evict(uint32_t L1_index, uint32_t L1_tag, uint32_t pa) {
  int L2_set_num = calc_set_num((L1_cache_size * 16), L1_cache_associativity, L1_cache_block_size);
  if (cache[0][L1_index][0].dirty) { // write-back to L2
    uint32_t L2_tag = convert_L1_tag_L2(cache[0][L1_index][0].tag, L1_index, L2_set_num);
    uint32_t L2_index = convert_L1_index_L2(cache[0][L1_index][0].tag, L1_index, L2_set_num);
    L2_write();
    write_to_L2_cache(pa);
    bool miss_flag_L2 = true;
    for (int j = 0; j < L1_cache_associativity; j++) {
      if ((cache[1][L2_index][j].tag == L2_tag) && (cache[1][L2_index][j].valid)) {
        cache[1][L2_index][j].dirty = true;
        L2_cache_write_hits += 1;
        L2_cache_hits += 1;
        miss_flag_L2 = false;
        LRUify_set(1, L2_index, j);
        break;
      }
    }
    if (miss_flag_L2) {
      L2_cache_misses += 1;
      L2_evict(L2_index, L2_tag, pa);
      cache[1][L2_index][L1_cache_associativity-1].dirty = true;
    }
  }
  cache[0][L1_index][0].valid = true;
  cache[0][L1_index][0].dirty = false;
  cache[0][L1_index][0].tag = L1_tag;
  LRUify_set(0, L1_index, 0);
}

void L1_write_evict(uint32_t L1_index, uint32_t L1_tag, uint32_t pa) {
  int L2_set_num = calc_set_num((L1_cache_size * 16), L1_cache_associativity, L1_cache_block_size);
  if (cache[0][L1_index][0].dirty) { // write-back to L2
    uint32_t L2_tag = convert_L1_tag_L2(cache[0][L1_index][0].tag, L1_index, L2_set_num);
    uint32_t L2_index = convert_L1_index_L2(cache[0][L1_index][0].tag, L1_index, L2_set_num);
    L2_write();
    write_to_L2_cache(pa);
    bool miss_flag_L2 = true;
    for (int j = 0; j < L1_cache_associativity; j++) {
      if ((cache[1][L2_index][j].tag == L2_tag) && (cache[1][L2_index][j].valid)) {
        cache[1][L2_index][j].dirty = true;
        L2_cache_write_hits += 1;
        L2_cache_hits += 1;
        miss_flag_L2 = false;
        LRUify_set(1, L2_index, j);
        break;
      }
    }
    if (miss_flag_L2) { // check if we've missed the search in L2 -- if so, add a miss and evict
        L2_cache_misses += 1; 
        L2_evict(L2_index, L2_tag, pa); // evict LRU of L2, replace with block brought in from mem
        cache[1][L2_index][L1_cache_associativity-1].dirty = true;
    }
  }
  cache[0][L1_index][0].valid = true;
  cache[0][L1_index][0].dirty = true;
  cache[0][L1_index][0].tag = L1_tag;
  LRUify_set(0, L1_index, 0);
}

void L2_read_op(uint32_t L1_index, uint32_t L1_tag, uint32_t L2_index, uint32_t L2_tag, uint32_t pa) {
  int L2_set_num = calc_set_num((L1_cache_size * 16), L1_cache_associativity, L1_cache_block_size);
  for (int i = 0; i < L2_set_num; i++) {
    if (i == L2_index) {
      for (int j = 0; j < L1_cache_associativity; j++) {
        if ((cache[1][i][j].tag == L2_tag) && cache[1][i][j].valid) {
          L2_cache_read_hits += 1;
          L2_cache_hits += 1;
          L1_evict(L1_index, L1_tag, pa);
          LRUify_set(1, L2_index, j);
          return;
        }
      }
      L2_cache_misses += 1; // L1 & L2 Miss, mem_read required
      mem_read();
      read_from_memory(pa);
      L2_evict(L2_index, L2_tag, pa);
      L1_evict(L1_index, L1_tag, pa);
    }
  }
}

void L2_write_read_op(uint32_t L1_index, uint32_t L1_tag, uint32_t L2_index, uint32_t L2_tag, uint32_t pa) {
  int L2_set_num = calc_set_num((L1_cache_size * 16), L1_cache_associativity, L1_cache_block_size);
  for (int i = 0; i < L2_set_num; i++) {
    if (i == L2_index) {
      for (int j = 0; j < L1_cache_associativity; j++) {
        if ((cache[1][i][j].tag == L2_tag) && cache[1][i][j].valid) {
          L2_cache_read_hits += 1;
          L2_cache_hits += 1;
          L1_write_evict(L1_index, L1_tag, pa);
          LRUify_set(1, L2_index, j);
          return;
        }
      }
      L2_cache_misses += 1;
      mem_read();
      read_from_memory(pa);
      L2_evict(L2_index, L2_tag, pa);
      L1_write_evict(L1_index, L1_tag, pa);
    }
  }
}

/*
 * Initialize the cache depending on the input parameters S, A, and B
 * and the statistics counter. The cache is declared in as extern in
 * include/cache.h file.
 */
void initialize_cache() {

    int L1_set_num = calc_set_num(L1_cache_size, L1_cache_associativity, L1_cache_block_size);
    int L2_set_num = calc_set_num((L1_cache_size * 16), L1_cache_associativity, L1_cache_block_size);

    cache = malloc(cache_level * sizeof(block_t **)); // malloc enough space for double pointers
    cache[0] = malloc(L1_set_num * sizeof(block_t *)); // malloc space for x sets of size (block_t *)
    for (int j = 0; j < L1_set_num; j++) {
      cache[0][j] = malloc(L1_cache_associativity*sizeof(block_t)); // malloc space for a block of size block_t
      for (int k = 0; k < (L1_cache_associativity); k++) { // k used to determine which block in a set we're accessing
        cache[0][j][k].dirty = false;
        cache[0][j][k].tag = 0;
        cache[0][j][k].valid = false; // initialise every block
      }
    }
    if (cache_level == 2) { // if multi-level cache, we also need to malloc in 2nd level
      cache[1] = malloc(L2_set_num * sizeof(block_t *));
      for (int j = 0; j < L2_set_num; j++) {
        cache[1][j] = malloc(L1_cache_associativity*sizeof(block_t));
        for (int k = 0; k < L1_cache_associativity; k++) {
          cache[1][j][k].dirty = false;
          cache[1][j][k].tag = 0;
          cache[1][j][k].valid = false;
        }
      }
    }
  return; }

/*
 * Free the allocated memory for the cache to avoid memory leaks.
 */
void free_cache() {
  int L1_set_num = calc_set_num(L1_cache_size, L1_cache_associativity, L1_cache_block_size);
  int L2_set_num = calc_set_num((L1_cache_size * 16), L1_cache_associativity, L1_cache_block_size); 
      for (int j = 0; j < L1_set_num; j++) {
        free(cache[0][j]); // work backwards in freeing cache mallocs cause segfault otherwise
      }
      free(cache[0]);
      if (cache_level == 2) {
        for (int j = 0; j < L2_set_num; j++) {
          free(cache[1][j]);
        }
        free(cache[1]);
      }
    free(cache);
  return; }

// Print cache statistics.
void print_cache_statistics() {
  printf("\n* Cache Statistics *\n");

  printf("memory total accesses: %d\n", memory_total_accesses);
  printf("memory read accesses: %d\n", memory_read_accesses);
  printf("memory write accesses: %d\n", memory_write_accesses);

  printf("L1 total accesses: %d\n", L1_cache_total_accesses);
  printf("L1 hits: %d\n", L1_cache_hits);
  printf("L1 misses: %d\n", L1_cache_misses);
  printf("L1 total reads: %d\n", L1_cache_read_accesses);
  printf("L1 read hits: %d\n", L1_cache_read_hits);
  printf("L1 total writes: %d\n", L1_cache_write_accesses);
  printf("L1 write hits: %d\n", L1_cache_write_hits);

  if (cache_level == 2) {
    printf("L2 total accesses: %d\n", L2_cache_total_accesses);
    printf("L2 hits: %d\n", L2_cache_hits);
    printf("L2 misses: %d\n", L2_cache_misses);
    printf("L2 total reads: %d\n", L2_cache_read_accesses);
    printf("L2 read hits: %d\n", L2_cache_read_hits);
    printf("L2 total writes: %d\n", L2_cache_write_accesses);
    printf("L2 write hits: %d\n", L2_cache_write_hits);
  }
}

/*
 * Perform a read from the cache for a particular address.
 * Since this is a simulation, there is no data. Ignore the data part.
 * The return value is always a HIT or a MISS and never an ERROR.
 */

op_result_t read_from_cache(uint32_t pa) { 
  int L1_set_num = calc_set_num(L1_cache_size, L1_cache_associativity, L1_cache_block_size);
  uint32_t index = calc_index(pa, L1_cache_size);
  uint32_t tag = calc_tag(pa, L1_cache_size);
  uint32_t L2_index = calc_index(pa, (L1_cache_size * 16));
  uint32_t L2_tag = calc_tag(pa, (L1_cache_size * 16));
  L1_read();
  bool miss_flag = false;
  for (int i = 0; i < L1_set_num; i++) {
    if (i == index) {
      for (int j = 0; j < L1_cache_associativity; j++) {
        if ((cache[0][i][j].tag == tag) && (cache[0][i][j].valid)) {
          L1_cache_read_hits += 1;
          L1_cache_hits += 1;
          LRUify_set(0, i, j); // carry newly-accessed block to right-most index in set (MRU)
          return HIT;
        } 
      }
      // miss, determie if just an L1 cache or multi-level
      L1_cache_misses += 1;
      miss_flag = true;
      if (cache_level == 2) {
        read_from_L2_cache(pa);
        L2_read();
        L2_read_op(index, tag, L2_index, L2_tag, pa);
      } else { // normal L1 handling if only single-level cache
        // If we get to this stage then we've missed and will have to evict based on LRU
        if (cache[0][i][0].dirty) { // LRU stored in index 0 of set
          write_to_memory(pa); // write to memory pa of dirty block
          mem_write();
        }
        read_from_memory(pa);
        mem_read();
        cache[0][i][0].tag = tag;
        cache[0][i][0].valid = true;
        cache[0][i][0].dirty = false; // not written to, dirty must be set to false
        LRUify_set(0, i, 0);
        return MISS;
      }
      if (miss_flag) {
        return MISS;
      }
  }
 }
 return MISS;
}

/*
 * Perform a write from the cache for a particular address.
 * Since this is a simulation, there is no data. Ignore the data part.
 * The return value is always a HIT or a MISS and never an ERROR.
 */
op_result_t write_to_cache(uint32_t pa) {
  int L1_set_num = calc_set_num(L1_cache_size, L1_cache_associativity, L1_cache_block_size);
  uint32_t index = calc_index(pa, L1_cache_size);
  uint32_t tag = calc_tag(pa, L1_cache_size);
  uint32_t L2_index = calc_index(pa, (L1_cache_size * 16));
  uint32_t L2_tag = calc_tag(pa, (L1_cache_size * 16));
  // cache follows write allocate and write back policies
  L1_write();
  bool miss_flag = false;

  for (int i = 0; i < L1_set_num; i++) {
    if (i == index) {
      for (int j = 0; j < L1_cache_associativity; j++) {
        if ((cache[0][i][j].tag == tag) && (cache[0][i][j].valid)) {
          L1_cache_write_hits += 1;
          L1_cache_hits += 1;
          cache[0][i][j].dirty = true;
          LRUify_set(0, i, j);
          return HIT;
        }
      }
      L1_cache_misses += 1;
      miss_flag = true;
      if (cache_level == 2) {
        read_from_L2_cache(pa);
        L2_read();
        L2_write_read_op(index, tag, L2_index, L2_tag, pa);    
      } else {
          // If we get to here, evict based on LRU policy
          if (cache[0][i][0].dirty) {
            write_to_memory(pa); // same as above, we're writing dirty block pa to memory
            mem_write();
          }
          read_from_memory(pa); // bring in from memory
          mem_read();
          cache[0][i][0].tag = tag;
          cache[0][i][0].valid = true;
          cache[0][i][0].dirty = true; // written to so dirty = true;
          LRUify_set(0, i, 0);
          return MISS;
      }
      if (miss_flag) {
        return MISS;
      }
  }
 }
  return MISS;
 }

// Process the S parameter properly and initialize `cache_size`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_S(int opt, char *optarg) { // process_arg functions only called if arg is supplied
   int size = atoi(optarg); // convert optarg to int
   if (check_pow_2(size)) {
    if (!((size < 4) || (size > 16384))) {
      L1_cache_size = size;
      return 0;
    }
   }
   return -1;
}

// Process the A parameter properly and initialize `cache_associativity`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_A(int opt, char *optarg) {
   int assoc = atoi(optarg);
   if (assoc < 1) { // lower bound check
    return -1;
   }
   if (check_pow_2(assoc)) {
      L1_cache_associativity = assoc;
      return 0;
   }
   return -1;
}

// Process the B parameter properly and initialize `cache_block_size`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_B(int opt, char *optarg) {
  int block_size = atoi(optarg);
  if (block_size < 4) { // lower bound check
    return -1;
  }
  if (check_pow_2(block_size)) {
      L1_cache_block_size = block_size;
      return 0;
  }
  return -1;
}

// Process the L parameter properly and initialize `cache_level`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_L(int opt, char *optarg) {
  int level = atoi(optarg);
  if ((level == 1) || (level == 2)) { // cache level can only be 1 or 2
    cache_level = level;
    return 0;
  }
  return -1;
}

// When verbose is true, print the details of each operation -- MISS or HIT.
void handle_cache_verbose(memory_access_entry_t entry, op_result_t ret) {
  if (ret == ERROR) {
    printf("This message should not be printed. Fix your code\n");
  }
}

// Check if all the necessary paramaters for the cache are provided and valid.
// Return 0 when everything is good. Otherwise return -1.
int check_cache_parameters_valid() {
  init_params(); // initialise default values if args not set
  // Perform check on arg_B
  if ((L1_cache_block_size < 4) || (L1_cache_block_size > L1_cache_size)) {
    return -1;
  }
  // Perform check on arg_A 
  if ((L1_cache_associativity < 1) || (L1_cache_associativity > (L1_cache_size / L1_cache_block_size))) {
    return -1;
  }
  // Task 4: Perform check on size of L1_cache if 2-level cache
  if ((cache_level == 2) && (L1_cache_size > 1024)) {
    return -1;
  }
  return 0;
 }

// *********************************************************
// BONUS TASK ONLY:
// Process the P parameter properly and initialize `cache_prefetching_policy`.
// Return 0 when everything is good. Otherwise return -1.
// *********************************************************
int process_arg_P(int opt, char *optarg) {
   if (strcmp("none", optarg) == 0) {
    cache_prefetching_policy = optarg;
    return 0;
   }
   if (strcmp("SEQ", optarg) == 0) {
    cache_prefetching_policy = optarg;
    return 0;
   }
   if (strcmp("STR", optarg) == 0) {
    cache_prefetching_policy = optarg;
    return 0;
   }
   if (strcmp("custom", optarg) == 0) {
    cache_prefetching_policy = optarg;
    return 0;
   }
   return -1;
   }
