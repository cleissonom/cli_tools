#ifndef SC_REPORT_RENDER_H
#define SC_REPORT_RENDER_H

#include "sc_report_types.h"

#include <stddef.h>
#include <stdio.h>

void report_render_summary (FILE *stream, const struct ReportSummary *summary);
void report_render_list (FILE *stream, const struct ReportEntry *entries,
                         size_t entry_count);
int report_export_entries_csv (const char *output_path,
                               const struct ReportEntry *entries,
                               size_t entry_count, char *error_output,
                               size_t error_output_size);
int report_export_entries_json (const char *output_path,
                                const struct ReportEntry *entries,
                                size_t entry_count, char *error_output,
                                size_t error_output_size);

#endif
