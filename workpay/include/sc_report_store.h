#ifndef SC_REPORT_STORE_H
#define SC_REPORT_STORE_H

#include "sc_report_types.h"

#include <stddef.h>

enum ReportStoreStatus
{
  REPORT_STORE_OK = 0,
  REPORT_STORE_PATH_ERROR,
  REPORT_STORE_READ_ERROR,
  REPORT_STORE_WRITE_ERROR,
  REPORT_STORE_PARSE_ERROR,
  REPORT_STORE_MEMORY_ERROR
};

int report_store_get_entries_file_path (char *output_path, size_t output_path_size);
void report_entry_list_free (struct ReportEntryList *entry_list);
enum ReportStoreStatus report_store_append_entry (const struct ReportEntry *entry,
                                                  char *error_output,
                                                  size_t error_output_size);
enum ReportStoreStatus report_store_load_entries (struct ReportEntryList *entry_list,
                                                  char *error_output,
                                                  size_t error_output_size);

#endif
