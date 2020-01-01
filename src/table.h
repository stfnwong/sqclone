/*
 * TABLE
 * Data structures for a table 
 *
 * Stefan Wong 2019
 */

#ifndef __SQ_TABLE_H
#define __SQ_TABLE_H

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100   // NOTE: this limit may change in future

#include <stdint.h>
#include <string.h>
/*
 * print_page_info()
 */
void print_page_info(void);

// Row data structure for DB
typedef struct 
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;


/*
 * print_row()
 */
void print_row(Row* row);
// TODO : row_to_str()? (bearing in mind that string 
// handling in C really sucks).


/* 
 * ROW STRUCTURE NOTES
 * There needs to be some structure that can represent a table. 
 * SQLite uses B-Trees for lookups but for now we use something 
 * simpler. The idea here is
 *
 *   - Store memory in blocks called pages.
 *   - Each page stores as many rows as it can fit.
 *   - Rows are serialized into a compact representation with
 *   each page.
 *   - Pages are only allocated as needed.
 *   - Keep a fixed-sized array of pointers to each page.
 */

// Compact representation of a Row 
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

void serialize_row(Row* src, void* dest);
void deserialize_row(void* src, Row* dst);

/* 
 * Table - structure that points to pages of rows
   and keeps track of how many rows there are
*/

// Actual table structure
typedef struct 
{
    uint32_t num_rows;
    uint32_t max_rows;
    void*    pages[TABLE_MAX_PAGES];
} Table;


Table* new_table(void);
void   free_table(Table* table);
void*  row_slot(Table* table, uint32_t row_num);


#endif /*__SQ_TABLE_H*/
