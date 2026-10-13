#include "common.h"

char *usage_str =
    "Usage: ./sim -t <trace_file> [-v] [-S <S>] [-B <B>] [-A <A>] [-L <L>]";

// Input parameters.
uint32_t verbose = 0;
char *trace_file = NULL;

// Read one entry at a time from the trace file.
memory_access_entry_t process_trace_file_line(FILE *trace_fp) {
  memory_access_entry_t entry;
  char operation;
  uint32_t address = 0;
  int ret = 0;

  ret = fscanf(trace_fp, " %c %x\n", &operation, &address);
  if (ret != EOF) {
    if (operation == 'R') {
      entry.address = address;
      entry.accesstype = READ;
    } else if (operation == 'W') {
      entry.address = address;
      entry.accesstype = WRITE;
    } else {
      entry.address = 0;
      entry.accesstype = INVALID;
    }
  } else {
    entry.address = 0;
    entry.accesstype = INVALID;
  }

  return entry;
}

uint32_t translate_address(memory_access_entry_t entry) {
  return (entry.address & 0x000FFFFF);
}
