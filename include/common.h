/*
 ********************************************************
  Please do not edit anything in this file.
 ********************************************************
*/

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdio.h>

extern char *usage_str;

extern uint32_t verbose;
extern char *trace_file;

typedef enum { READ, WRITE, INVALID } access_t;

typedef enum { MISS, HIT, ERROR } op_result_t;

typedef struct {
  uint32_t address;
  access_t accesstype;
} memory_access_entry_t;

memory_access_entry_t process_trace_file_line(FILE *trace_fp);

uint32_t translate_address(memory_access_entry_t entry);

#endif /* COMMON_H_ */
