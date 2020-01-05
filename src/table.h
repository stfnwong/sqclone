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

// Attribute size
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

// Constants for Table structure
#define ID_SIZE          size_of_attribute(Row, id)
#define USERNAME_SIZE    size_of_attribute(Row, username)
#define EMAIL_SIZE       size_of_attribute(Row, email)
#define ID_OFFSET        0
#define USERNAME_OFFSET  (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET     (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE         (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

/* 
 * The above means that we have the following layout in memory
 *
 *  column      size (bytes)     offset 
 *  id          4                0
 *  username    32               4
 *  email       255              36
 *  total       291
 */
#define PAGE_SIZE 4096        // same as OS VM page size 
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
void serialize_row(Row* src, void* dest);
void deserialize_row(void* src, Row* dst);

/*
 * Pager
 * Object that accesses the cache and file. Tables make 
 * requests for pages through the pager.
 */
typedef struct
{
    int      fd;     // file descriptor
    uint32_t file_length;
    uint32_t num_pages;
    void*    pages[TABLE_MAX_PAGES];
} Pager;

Pager* pager_open(const char* filename);
void   pager_flush(Pager* pager, uint32_t page_num);
void*  get_page(Pager* pager, uint32_t page_num);

/* 
 * Table - structure that points to pages of rows
   and keeps track of how many rows there are
*/
typedef struct 
{
    uint32_t root_page_num;
    //uint32_t max_rows;
    Pager*   pager;
} Table;

Table* db_open(const char* filename);
void   db_close(Table* table);


/*
 * Cursor
 * Represents a location in a table.
 */
typedef struct 
{
    Table*   table;
    uint32_t page_num;
    uint32_t cell_num;
    int      end_of_table;  // this is a position one-past the last element
} Cursor;

Cursor* table_start(Table* table);
Cursor* table_end(Table* table);
void*   cursor_value(Cursor* cursor);
void    cursor_advance(Cursor* cursor);

/*
 * Common Node Header Layout
 */
#define NODE_TYPE_SIZE           sizeof(uint8_t)
#define NODE_TYPE_OFFSET         0
#define IS_ROOT_SIZE             sizeof(uint8_t)
#define IS_ROOT_OFFSET           NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE      sizeof(uint32_t)
#define PARENT_POINTER_OFFSET    (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE  (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

/*
 * Leaf Node Header Layout
 */
#define LEAF_NODE_NUM_CELLS_SIZE   sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE      (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE)
/*
 * Leaf Node Body Layout
 */
#define LEAF_NODE_KEY_SIZE        sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET      0
#define LEAF_NODE_VALUE_SIZE      ROW_SIZE     
#define LEAF_NODE_VALUE_OFFSET    (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE       (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS       (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)


/*
 * Tree Nodes
 */
uint32_t* leaf_node_num_cells(void* node);
void*     leaf_node_cell(void* node, uint32_t cell_num);
uint32_t* leaf_node_key(void* node, uint32_t cell_num);
void*     leaf_node_value(void* node, uint32_t cell_num);
void      init_leaf_node_value(void* node);
void      leaf_node_insert(Cursor* cursor, uint32_t key, Row* value);
void      print_leaf_node(void* node);

void print_info(void);


#endif /*__SQ_TABLE_H*/
