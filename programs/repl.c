/*
 * REPL
 * Implementation of an SQLite-like REPL
 * I'm mainly following along from (https://cstack.github.io/db_tutorial/parts/part1.html), which is where the real credit should go.
 *
 * Stefan Wong 2019
 */

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

#include <string.h>     // for memcpy()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Row data structure for DB
typedef struct 
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;


void print_row(Row* row)
{
    fprintf(stdout, "(%d, %s, %s)\n", 
            row->id,
            row->username,
            row->email
    );
}

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


// Table - structure that points to pages of rows
// and keeps track of how many rows there are

const uint32_t PAGE_SIZE = 4096;        // same as OS VM page size 
#define TABLE_MAX_PAGES 100             // NOTE: this limit may change in future
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct 
{
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;

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


// Execution stuff
typedef enum 
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;


// Input buffer structure
typedef struct 
{
    char*   buffer;
    size_t  buffer_length;
    ssize_t input_length;
} InputBuffer;

/*
 * new_input_buffer()
 */
InputBuffer* new_input_buffer(void)
{
    InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
    if(!input_buffer)
    {
        fprintf(stderr, "[%s] Failed to allocate memory for Input buffer\n", __func__);
        return NULL;
    }

    input_buffer->buffer        = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length  = 0;

    return input_buffer;
}

/*
 * close_input_buffer()
 * Close a buffer and free associated memory
 */
void close_input_buffer(InputBuffer* input_buffer)
{
    free(input_buffer->buffer);
    free(input_buffer);
}

// Metacommand stuff 
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum 
{
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;


/*
 * do_meta_command()
 * Handle metacommands here
 */
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table)
{
    if(strncmp(input_buffer->buffer, ".exit", 6) == 0)
    {
        close_input_buffer(input_buffer);
        free_table(table);
        exit(EXIT_SUCCESS);
    }
    else
        return META_COMMAND_UNRECOGNIZED_COMMAND;
}

// Statement stuff 
typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct
{
    StatementType type;
    Row row_to_insert;      // only used by insert statement
} Statement;

/*
 * prepare_statement()
 */
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement)
{
    if(strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        int args_assigned;

        statement->type = STATEMENT_INSERT;
        args_assigned = sscanf(
                input_buffer->buffer, "insert %d %s %s",
                &(statement->row_to_insert.id),
                statement->row_to_insert.username,
                statement->row_to_insert.email
        );
        // Catch incorrect numbers of arguments
        if(args_assigned < 3)
        {
            return PREPARE_SYNTAX_ERROR;
        }

        return PREPARE_SUCCESS;
    }

    if(strncmp(input_buffer->buffer, "select", 6) == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

/*
 * execute_statement()
 */
ExecuteResult execute_insert(Statement* statement, Table* table)
{
    if(table->num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows++;

    return EXECUTE_SUCCESS;
}

/*
 * execute_select()
 */
ExecuteResult execute_select(Statement* statement, Table* table)
{
    Row row;
    for(uint32_t i = 0; i < table->num_rows; ++i)
    {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }

    return EXECUTE_SUCCESS;
}

/*
 * execute_statement()
 */
ExecuteResult execute_statement(Statement* statement, Table* table)
{
    switch(statement->type)
    {
        case STATEMENT_INSERT:
            return execute_insert(statement, table);

        case STATEMENT_SELECT:
            return execute_select(statement, table);
    }

    // So far, nothing could go wrong here so there is no error handling. 
    // As the implementation becomes more complete there will need to be 
    // additional logic here to take care of possible errors.
}

/*
 * print_prompt()
 */
void print_prompt(void)
{
    fprintf(stdout, "db > ");
}

/*
 * read_input()
 * Read some input from stdin.
 */
void read_input(InputBuffer* input_buffer)
{
    ssize_t bytes_read;

    bytes_read = getline(
            &(input_buffer->buffer), 
            &(input_buffer->buffer_length),
            stdin
    );

    if(bytes_read <= 0)
    {
        fprintf(stderr, "Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // Ignore trailing newline
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
}


// Entry point
int main(int argc, char *argv[])
{
    InputBuffer* input_buffer = new_input_buffer();
    Statement statement;
    Table* table;

    table = new_table();
    if(!table)
    {
        fprintf(stderr, "[%s] failed to allocate memory for table\n", __func__);
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        print_prompt();
        read_input(input_buffer);

        // Commands start with '.' character
        if(input_buffer->buffer[0] == '.')
        {
            switch(do_meta_command(input_buffer, table))
            {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    fprintf(stdout, "Unrecognized command [%s].\n", input_buffer->buffer);
                    continue;
            }
        }

        // If not a command, parse as a statement
        switch(prepare_statement(input_buffer, &statement))
        {
            case PREPARE_SUCCESS:
                break;

            case PREPARE_SYNTAX_ERROR:
                fprintf(stdout, "Syntax error in statement [%s]\n", input_buffer->buffer);
                continue;

            case PREPARE_UNRECOGNIZED_STATEMENT:
                fprintf(stdout, "Unrecognized keyword at start of [%s]\n",
                        input_buffer->buffer
                );
                continue;
        }

        // Execute the statement
        switch(execute_statement(&statement, table))
        {
            case EXECUTE_SUCCESS:
                fprintf(stdout, "Executed [%s]\n", input_buffer->buffer);
                break;

            case EXECUTE_TABLE_FULL:
                fprintf(stdout, "ERROR: Table full\n");
                break;
        }
    }

    return 0;
}
