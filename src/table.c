/*
 * TABLE
 * Data structures for a table 
 *
 * Stefan Wong 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include "table.h"


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
    memcpy(dest + USERNAME_OFFSET, &(src->username), USERNAME_SIZE);
    memcpy(dest + EMAIL_OFFSET, &(src->email), EMAIL_SIZE);
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
 * new_table()
 */
Table* new_table(void)
{
    Table* table = malloc(sizeof(Table));
    if(!table)
        return NULL;

    for(uint32_t i = 0; i < TABLE_MAX_PAGES; ++i)
    {
        table->pages[i] = NULL;
    }

    return table;
}

/*
 * free_table()
 */
void free_table(Table* table)
{
    for(int i = 0; table->pages[i] != NULL; ++i)
        free(table->pages[i]);

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

    page_num = row_num / ROWS_PER_PAGE;
    page = table->pages[page_num];

    // Only allocate when we try to access a page
    if(page == NULL)
    {
        table->pages[page_num] = malloc(PAGE_SIZE);
        page = table->pages[page_num];
    }

    row_offset = row_num % ROWS_PER_PAGE;
    byte_offset = row_offset * ROW_SIZE;

    return page + byte_offset;
}
