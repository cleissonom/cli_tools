#ifndef SC_REPORT_QUERY_H
#define SC_REPORT_QUERY_H

#include "sc_report_types.h"

#include <stddef.h>

void report_filter_init (struct ReportFilter *filter);
int report_validate_filter (const struct ReportFilter *filter,
                            char *error_output, size_t error_output_size);
void report_summary_free (struct ReportSummary *summary);
int report_filter_entries (const struct ReportEntry *entries, size_t entry_count,
                           const struct ReportFilter *filter,
                           struct ReportEntryList *filtered_entries,
                           char *error_output, size_t error_output_size);
int report_build_summary (const struct ReportEntry *entries, size_t entry_count,
                          struct ReportSummary *summary,
                          char *error_output, size_t error_output_size);

#endif
