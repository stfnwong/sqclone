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
#include "table.h"

// Constants for Table structure
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

/* 
 * The above means that we have the following layout in memory
 *
 *  column      size (bytes)     offset 
 *  id          4                0
 *  username    32               4
 *  email       255              36
 *  total       291
 */
const uint32_t PAGE_SIZE      = 4096;        // same as OS VM page size 
const uint32_t ROWS_PER_PAGE  = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

/*
 * print_page_info()
 */
void print_page_info(void)
{
    fprintf(stdout, "ROW_SIZE        : %d\n", ROW_SIZE);
    fprintf(stdout, "PAGE_SIZE       : %d\n", PAGE_SIZE);
    fprintf(stdout, "ROWS_PER_PAGE   : %d\n", ROWS_PER_PAGE);
    fprintf(stdout, "TABLE_MAX_ROWS  : %d\n", TABLE_MAX_ROWS);
    fprintf(stdout, "TABLE_MAX_PAGES : %d\n", TABLE_MAX_PAGES);
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

    for(uint32_t p = 0; p < TABLE_MAX_PAGES; ++p)
        pager->pages[p] = NULL;

    return pager;
}

/*
 * pager_flush()
 */
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size)
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
            size
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
    }

    return pager->pages[page_num];
}

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
    table->max_rows = TABLE_MAX_ROWS;
    table->num_rows = pager->file_length / ROW_SIZE;
    table->pager    = pager;

    return table;
}

/*
 * db_close()
 */
void db_close(Table* table)
{
    Pager* pager;
    uint32_t num_full_pages;
    uint32_t num_extra_rows;

    pager = table->pager;
    num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for(uint32_t p = 0; p < num_full_pages; ++p)
    {
        if(pager->pages[p] == NULL)
            continue;

        pager_flush(pager, p, PAGE_SIZE);
        free(pager->pages[p]);
        pager->pages[p] = NULL;
    }

    // There may be a partial page write to the end of the file .
    // The need to check for this will change when the implementation
    // is moved to B-Trees.
    num_extra_rows = table->num_rows % ROWS_PER_PAGE;
    if(num_extra_rows > 0)
    {
        uint32_t page_num = num_full_pages;
        if(pager->pages[page_num] != NULL)
        {
            pager_flush(pager, page_num, num_extra_rows * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
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

/*
 * row_slot()
 * Figure out where to read/write in memory for a particular row 
 */
void* row_slot(Table* table, uint32_t row_num)
{
    uint32_t page_num, row_offset, byte_offset;
    void* page;

    page_num    = row_num / ROWS_PER_PAGE;
    page        = get_page(table->pager, page_num);
    row_offset  = row_num % ROWS_PER_PAGE;
    byte_offset = row_offset * ROW_SIZE;

    return page + byte_offset;
}
