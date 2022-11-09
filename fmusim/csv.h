/* (c) 2019 Jan Doczy
 * This code is licensed under MIT license (see LICENSE.txt for details) */

/* simple and fast CSV reader:
 * 1. Open CSV file by calling CsvOpen("filename.csv")
 * 2. Read CSV row by calling CsvReadNextRow(csv_handle)
 * 3. Read single CSV line column by calling CsvReadNextCol(returned_row_str, csv_handle)
 */

#ifndef CSV_H_INCLUDED
#define CSV_H_INCLUDED

#ifdef __cplusplus
extern "C" {  /* C++ name mangling */
#endif

/* pointer to private handle structure */
typedef struct CsvHandle_ *CsvHandle;

/**
 * openes csv file
 * @filename: pathname of the file
 * @return: csv handle
 * @notes: you should call CsvClose() to release resources
 */
CsvHandle CsvOpen(const char* filename);
CsvHandle CsvOpen2(const char* filename,
                   char delim,
                   char quote,
                   char escape);

/**
 * closes csv handle, releasing all resources
 * @handle: csv handle
 */
void CsvClose(CsvHandle handle);

/**
 * reads (first / next) line of csv file
 * @handle: csv handle
 */
char* CsvReadNextRow(CsvHandle handle);

/**
 * get column of file
 * @row: csv row (you can use CsvReadNextRow() to parse next line)
 * @context: handle returned by CsvOpen() or CsvOpen2()
 */
const char* CsvReadNextCol(char* row, CsvHandle handle);

#ifdef __cplusplus
};
#endif

#endif
