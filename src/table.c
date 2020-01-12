/*
 * TABLE
 * Data structures for a table 
 *
 * Stefan Wong 2019
 */

#include <errno.h>
// For opening fd
#include <sys/stat.h>
#include <unistd.h>        // for closing fd
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"


// ================ ROW

/*
 * print_page_info()
 */
void print_page_info(void)
{
    fprintf(stdout, "ROW_SIZE        : %ld\n", ROW_SIZE);
    fprintf(stdout, "PAGE_SIZE       : %ld\n", PAGE_SIZE);
    fprintf(stdout, "TABLE_MAX_PAGES : %ld\n", TABLE_MAX_PAGES);
}

/*
 * print_row()
 */
void print_row(Row* row)
{
    fprintf(stdout, "(%d, %s, %s)\n", 
            row->id,
            row->username,
            row->email
    );
}

/*
 * serialize_row()
 */
void serialize_row(Row* src, void* dest)
{
    memcpy(dest + ID_OFFSET, &(src->id), ID_SIZE);
    // we use strncpy here to ensure that all bytes are
    // initialized to zeros
    strncpy(dest + USERNAME_OFFSET, src->username, USERNAME_SIZE);
    strncpy(dest + EMAIL_OFFSET, src->email, USERNAME_SIZE);
}

/*
 * deserialize_row()
 */
void deserialize_row(void* src, Row* dst)
{
    memcpy(&(dst->id), src + ID_OFFSET, ID_SIZE);
    memcpy(&(dst->username), src + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(dst->email), src + EMAIL_OFFSET, EMAIL_SIZE);
}


// ================ TREE NODES
typedef enum
{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;


/*
 * Leaf Node methods
 * All of these methods find elements in the B+Tree based on pointer offsets
 */
uint32_t* leaf_node_num_cells(void* node)
{
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num)
{
    return node + LEAF_NODE_HEADER_SIZE +
        cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num)
{
    return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num)
{
    return leaf_node_cell(node, cell_num) + 
        LEAF_NODE_KEY_SIZE;
}

void init_leaf_node_value(void* node)
{
    *leaf_node_num_cells(node) = 0;
}


// ================ PAGER

/*
 * pager_open()
 */
Pager* pager_open(const char* filename)
{
    int    fd;
    Pager* pager;

    fd = open(
            filename, 
            O_RDWR | O_CREAT,       // read/write mode, create if file does not exist
            S_IWUSR | S_IRUSR       // user read permission, user write permission
    );

    if(fd == -1)
    {
        fprintf(stdout, "[%s] unable to open file %s\n", __func__, filename);
        return NULL;
    }

    // setup pager
    pager = malloc(sizeof(Pager));
    pager->fd = fd;
    pager->file_length = lseek(fd, 0, SEEK_END);
    pager->num_pages   = (pager->file_length / PAGE_SIZE);

    if(pager->file_length % PAGE_SIZE != 0)
    {
        fprintf(stderr, "[%s] Corruption: DB file is a not a whole number of pages\n", __func__);
        return NULL;
    }

    for(uint32_t p = 0; p < TABLE_MAX_PAGES; ++p)
        pager->pages[p] = NULL;

    return pager;
}

/*
 * pager_flush()
 */
void pager_flush(Pager* pager, uint32_t page_num)
{
    ssize_t bytes_written;

    if(pager->pages[page_num] == NULL)
    {
        fprintf(stdout, "[%s] tried to flush null page %d\n", __func__, page_num);
        return;
    }

    bytes_written = write(
            pager->fd, 
            pager->pages[page_num],
            PAGE_SIZE
    );

    if(bytes_written == -1)
    {
        fprintf(stdout, "[%s] error writing [errno: %d]\n", __func__, errno);
        return;     // EXIT_FAILURE?
    }
}

/*
 * get_page()
 */
void* get_page(Pager* pager, uint32_t page_num)
{
    if(page_num > TABLE_MAX_PAGES)
    {
        fprintf(stdout, "[%s] page %d out of bounds (max page %d)\n",
                __func__, page_num, TABLE_MAX_PAGES);
        return NULL;
    }

    if(pager->pages[page_num] == NULL)
    {
        // cache miss - allocate new memory
        void* page;
        uint32_t num_pages;

        page = malloc(PAGE_SIZE);
        if(!page)
        {
            fprintf(stdout, "[%s] failed to allocate memory for page %d\n", __func__, page_num);
            return NULL;
        }

        num_pages = pager->file_length / PAGE_SIZE;
        // account for partial pages saved at the end of the file
        if(pager->file_length % PAGE_SIZE)
            num_pages++;

        if(page_num < num_pages)
        {
            lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->fd, page, PAGE_SIZE);
            if(bytes_read == -1)
            {
                fprintf(stdout, "[%s] Error reading file [error %d]\n", __func__, errno);
                return NULL;
            }
        }
        pager->pages[page_num] = page;

        if(page_num > pager->num_pages)
            pager->num_pages = page_num + 1;
    }

    return pager->pages[page_num];
}

// ================ TABLE

/*
 * db_open()
 */
Table* db_open(const char* filename)
{
    Pager* pager;
    Table* table;

    pager = pager_open(filename);
    if(pager == NULL)
    {
        fprintf(stderr, "[%s] failed to create pager object for table with db file [%s]\n",
                __func__, filename);
        return NULL;
    }

    table = malloc(sizeof(Table));
    if(!table)
    {
        fprintf(stderr, "[%s] failed to allocate memory for table with db file [%s]\n",
                __func__, filename);
        return NULL;
    }
    table->root_page_num = 0;
    table->pager         = pager;

    // If this is a new db file then init page 0 as a leaf node
    if(pager->num_pages == 0)
    {
        void* root_node;

        root_node = get_page(pager, 0);
        init_leaf_node_value(root_node);
    }

    return table;
}

/*
 * db_close()
 */
void db_close(Table* table)
{
    Pager* pager;

    pager = table->pager;
    for(uint32_t p = 0; p < pager->num_pages; ++p)
    {
        if(pager->pages[p] == NULL)
            continue;

        pager_flush(pager, p);
        free(pager->pages[p]);
        pager->pages[p] = NULL;
    }

    int result = close(pager->fd);      // <- TODO : segfault here
    if(result == -1)
    {
        fprintf(stdout, "[%s] Error closing db file\n", __func__);
        exit(EXIT_FAILURE);
    }

    for(uint32_t p = 0; p < TABLE_MAX_PAGES; ++p)
    {
       void* page = pager->pages[p];
       if(page)
       {
           free(page);
           pager->pages[p] = NULL;
       }
    }

    free(pager);
    free(table);
}



// ================ CURSOR

/*
 * table_start()
 */
Cursor* table_start(Table* table)
{
    void*   root_node;
    Cursor* cursor;

    cursor = malloc(sizeof(Cursor));
    if(!cursor)
    {
        fprintf(stderr, "[%s] failed to allocate memory for Cursor object\n", __func__);
        return NULL;
    }

    cursor->table    = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    root_node = get_page(table->pager, table->root_page_num);
    cursor->end_of_table = (*leaf_node_num_cells(root_node) == 0) ? 1 : 0;

    return cursor;
}

/*
 * table_end()
 */
Cursor* table_end(Table* table)
{
    void* root_node;
    Cursor* cursor;

    cursor = malloc(sizeof(Cursor));
    if(!cursor)
    {
        fprintf(stderr, "[%s] failed to allocate memory for Cursor object\n", __func__);
        return NULL;
    }

    cursor->table        = table;
    cursor->page_num     = table->root_page_num;
    cursor->end_of_table = 1;

    root_node        = get_page(table->pager, table->root_page_num);
    cursor->cell_num = *leaf_node_num_cells(root_node);

    return cursor;
}

/*
 * cursor_value()
 * Figure out where to read/write in memory for a particular row 
 */
void* cursor_value(Cursor* cursor)
{
    void* page;

    page = get_page(cursor->table->pager, cursor->page_num);

    return leaf_node_value(page, cursor->cell_num);
}

/*
 * cursor_advance()
 */
void cursor_advance(Cursor* cursor)
{
    void* node;

    node = get_page(cursor->table->pager, cursor->page_num);
    cursor->cell_num++;
    if(cursor->cell_num >= (*leaf_node_num_cells(node)))
        cursor->end_of_table = 1;
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value)
{
    void*    node;
    uint32_t num_cells;

    node      = get_page(cursor->table->pager, cursor->page_num);
    num_cells = *leaf_node_num_cells(node);
    // check if node is full
    if(num_cells >= LEAF_NODE_MAX_CELLS)
    {
        fprintf(stdout, "[%s] Leaf not splitting not yet implemented\n", __func__);
        return;
    }

    if(cursor->cell_num < num_cells)
    {
        // make room for a new cell
        for(uint32_t i = 0; i < cursor->cell_num; ++i)
        {
            memcpy(
               leaf_node_cell(node, i),
               leaf_node_cell(node, i-1),
               LEAF_NODE_CELL_SIZE
           );
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}
