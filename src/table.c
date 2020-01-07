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
    fprintf(stdout, "PAGE_SIZE       : %d\n",  PAGE_SIZE);
    fprintf(stdout, "TABLE_MAX_PAGES : %d\n",  TABLE_MAX_PAGES);
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

        if(page_num >= pager->num_pages)
            pager->num_pages = page_num + 1;
    }

    return pager->pages[page_num];
}

/*
 * get_unused_page_num()
 * Until we implement page recycling new pages will always go 
 * on the end of the database.
 */
uint32_t get_unused_page_num(Pager* pager)
{
    return pager->num_pages;
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
        init_leaf_node(root_node);
    }

    return table;
}

/*
 * db_close()
 */
void db_close(Table* table)
{
    int result;
    Pager* pager;

    pager = table->pager;
    // Write out all the pages 
    for(uint32_t p = 0; p < pager->num_pages; ++p)
    {
        if(pager->pages[p] == NULL)
            continue;

        pager_flush(pager, p);
        free(pager->pages[p]);
        pager->pages[p] = NULL;
    }

    // Close the file descriptor
    result = close(pager->fd);
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
 * table_find()
 * Return the position of the node given by key if it exists, else
 * return the position where the node given by key should be 
 * inserted.
 */
Cursor* table_find(Table* table, uint32_t key)
{
    void* root_node;

    root_node = get_page(table->pager, table->root_page_num);
    if(get_node_type(root_node) == NODE_LEAF)
        return leaf_node_find(table, table->root_page_num, key);
    else
    {
        fprintf(stdout, "[%s] Need to implement searching an internal node\n", __func__);
        return NULL;
    }
}

/*
 * cursor_value()
 * Figure out where to read/write in memory for a particular row 
 */
void* cursor_value(Cursor* cursor)
{
    uint32_t page_num;
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

void init_leaf_node(void* node)
{
    set_node_type(node, NODE_LEAF);
    set_node_root(node, 0);
    (*leaf_node_num_cells(node)) = 0;
}

/*
 * leaf_node_insert()
 */
void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value)
{
    void*    node;
    uint32_t num_cells;

    node      = get_page(cursor->table->pager, cursor->page_num);
    num_cells = (*leaf_node_num_cells(node));
    // check if node is full
    if(num_cells >= LEAF_NODE_MAX_CELLS)
    {
        leaf_node_split_and_insert(cursor, key, value);
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

/*
 * leaf_node_split_and_insert()
 */
void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value)
{
    // Create a new node and move half of the cells over. 
    // Insert the new value into one of two nodes. Update
    // parent or create new parent.

    void*    old_node;
    void*    new_node;
    uint32_t new_page_num;

    old_node     = get_page(cursor->table->pager, cursor->page_num);
    new_page_num = get_unused_page_num(cursor->table->pager);
    new_node     = get_page(cursor->table->pager, new_page_num);

    // Now we divide all existing keys evenly between old (left) 
    // and new (right) nodes. Starting from the right, move each
    // key to correct position.

    uint32_t idx_within_node;
    for(int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; --i)
    {
        void* dest_node;

        if(i >= LEAF_NODE_LEFT_SPLIT_COUNT)
            dest_node = new_node;
        else
            dest_node = old_node;

        idx_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        void* dest = leaf_node_cell(dest_node, idx_within_node);

        if(i == cursor->cell_num)
            serialize_row(value, dest);
        else if(i > cursor->cell_num)
            memcpy(dest, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        else
            memcpy(dest, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }

    // Update node counts on both leaf nodes
    *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

    // Update the nodes parent. If the original node was the root 
    // then it had no parent. In this case, we create a new root
    // node to act as the parent.

    // TODO : note that this is just a stub
    if(is_node_root(old_node))
        return create_new_root(cursor->table, new_page_num);
    else
    {
        fprintf(stdout, "[%s] Updating parent after split not yet implemented\n",
                __func__);
        return ;
    }

}

/*
 * leaf_node_find()
 */
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key)
{
    Cursor*  cursor;
    void*    node;
    uint32_t num_cells;

    node = get_page(table->pager, page_num);
    cursor = malloc(sizeof(Cursor));
    if(!cursor)
    {
        fprintf(stderr, "[%s] failed to allocate memory for Cursor object\n", __func__);
        return NULL;
    }
    num_cells = (*leaf_node_num_cells(node));

    cursor->table    = table;
    cursor->page_num = page_num;

    // Do a binary search over the leaf node
    uint32_t min_idx, one_past_max_idx;

    min_idx = 0;
    one_past_max_idx = num_cells;

    while(one_past_max_idx != min_idx)
    {
        uint32_t idx = (min_idx + one_past_max_idx) / 2;
        uint32_t key_at_idx = (*leaf_node_key(node, idx));
        if(key == key_at_idx)
        {
            cursor->cell_num = idx;
            return cursor;
        }

        if(key < key_at_idx)
            one_past_max_idx = idx;
        else
            min_idx++;
    }
    cursor->cell_num =  min_idx;

    return cursor;
}

/*
 * get_node_type()
 */
NodeType get_node_type(void* node)
{
    uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));

    return (NodeType) value;
}

/*
 * set_node_type()
 */
void set_node_type(void* node, NodeType type)
{
    uint8_t value;

    value = (uint8_t) type;
    *((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

/*
 * set_node_root()
 */
void set_node_root(void* node, uint8_t is_root)
{
    *((uint8_t*)(node + IS_ROOT_OFFSET)) = is_root;
}

/*
 * is_node_root()
 */
uint8_t is_node_root(void* node)
{
    uint8_t value;

    value = *((uint8_t*)(node + IS_ROOT_OFFSET));
    
    return value;
}


/*
 * create_new_root()
 */
void create_new_root(Table* table, uint32_t right_child_page_num)
{
    // Handle splitting the root node.
    // Copy old root to new page, this becomes the left child.
    // Address of right child is passed as parameter
    // Re-init the root page to contain the root node.
    // New root node points to two children.

    void* root;
    void* right_child;
    void* left_child;
    uint32_t left_child_page_num;

    root        = get_page(table->pager, table->root_page_num);
    right_child = get_page(table->pager, right_child_page_num);
    left_child_page_num = get_unused_page_num(table->pager);
    left_child  = get_page(table->pager, left_child_page_num);

    // left child has data copied from old root
    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, 1);        // can go back and use stdbool
}


/*
 * print_node()
 */
void print_leaf_node(void* node)
{
    uint32_t num_cells;

    num_cells = (*leaf_node_num_cells(node));
    fprintf(stdout, "[%s] leaf size: %d cells\n", __func__, num_cells);
    fprintf(stdout, "   cell | key\n");

    for(uint32_t i = 0; i < num_cells; ++i)
    {
        uint32_t key = *leaf_node_key(node, i);
        fprintf(stdout, "   - %d : %d\n", i, key);
    }
}


/*
 * Internal nodes
 */
// Header layout
#define INTERNAL_NODE_NUM_KEYS_SIZE    sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET  COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE)

// Body layout
#define INTERNAL_NODE_KEY_SIZE   sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE  (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)

// Methods for accessing internal node structures
/*
 * internal_node_num_keys()
 */
uint32_t* internal_node_num_keys(void* node)
{
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

/*
 * internal_node_right_child()
 */
uint32_t* internal_node_right_child(void* node)
{
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

/* 
 * internal_node_cell()
 */
uint32_t* internal_node_cell(void* node, uint32_t cell_num)
{
    return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

/*
 * internal_node_child()
 */
uint32_t* internal_node_child(void* node, uint32_t child_num)
{
    uint32_t num_keys;

    num_keys = (*internal_node_num_keys(node));
    if(child_num > num_keys)
    {
        fprintf(stdout, "[%s] tried to access child %d in node with %d keys\n",
                __func__, child_num, num_keys);
        return NULL;
    }
    else if(child_num == num_keys)
        return internal_node_right_child(node);
    else
        return internal_node_cell(node, child_num);
}

/*
 * internal_node_key()
 */
uint32_t* internal_node_key(void* node, uint32_t key_num)
{
    return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}


/*
 * init_internal_node()
 */
void init_internal_node(void* node)
{
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, 0);
    *internal_node_num_keys(node) = 0;
}

/*
 * get_node_max_key()
 */
uint32_t get_node_max_key(void* node)
{
    switch(get_node_type(node))
    {
        case NODE_INTERNAL:
            return *internal_node_key(node, *internal_node_num_keys(node) - 1);
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
        default:
            // Should never get this
            fprintf(stderr, "[%s] invalid node type %d\n",
                    __func__, get_node_type(node));
    }
}

/*
 * print_info()
 */
void print_info(void)
{
    fprintf(stdout, "ROW_SIZE                  : %ld\n", ROW_SIZE);
    fprintf(stdout, "COMMON_NODE_HEADER_SIZE   : %ld\n",  COMMON_NODE_HEADER_SIZE);
    fprintf(stdout, "LEAF_NODE_HEADER_SIZE     : %ld\n",  LEAF_NODE_HEADER_SIZE);
    fprintf(stdout, "LEAF_NODE_CELL_SIZE       : %ld\n",  LEAF_NODE_CELL_SIZE);
    fprintf(stdout, "LEAF_NODE_SPACE_FOR_CELLS : %ld\n",  LEAF_NODE_SPACE_FOR_CELLS);
    fprintf(stdout, "LEAF_NODE_MAX_CELLS       : %ld\n",  LEAF_NODE_MAX_CELLS);
}
