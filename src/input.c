/*
 * INPUT
 * Input handling functions 
 *
 * Stefan Wong 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "input.h"
#include "table.h"

/*
 * repl_print_prompt()
 */
void repl_print_prompt(void)
{
    fprintf(stdout, "db > ");
}

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

/*
 * do_meta_command()
 * Handle metacommands here
 */
MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table)
{
    if(strncmp(input_buffer->buffer, ".exit", 6) == 0)
    {
        db_close(table);
        close_input_buffer(input_buffer);
        exit(EXIT_SUCCESS);
    }
    else if(strncmp(input_buffer->buffer, ".info", 5) == 0)
    {
        print_info();
        return META_COMMAND_SUCCESS;
    }
    else if(strncmp(input_buffer->buffer, ".btree", 6) == 0)
    {
        print_leaf_node(get_page(table->pager, 0));
        return META_COMMAND_SUCCESS;
    }
    else
        return META_COMMAND_UNRECOGNIZED_COMMAND;
}

/*
 * prepare_insert()
 */
PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement)
{
    char* keyword;
    char* id_string;
    char* username;
    char* email;
    int   id;

    statement->type = STATEMENT_INSERT;
    keyword         = strtok(input_buffer->buffer, " ");
    id_string       = strtok(NULL, " ");
    username        = strtok(NULL, " ");
    email           = strtok(NULL, " ");

    // check what strings we've received
    if(id_string == NULL || username == NULL || email == NULL)
    {
        return PREPARE_SYNTAX_ERROR;  
    }

    id = atoi(id_string);
    if(id < 0)
        return PREPARE_NEGATIVE_ID;

    // Check lengths
    if(strlen(username) > COLUMN_USERNAME_SIZE)
        return PREPARE_STRING_TOO_LONG;
    if(strlen(email) > COLUMN_EMAIL_SIZE)
        return PREPARE_STRING_TOO_LONG;

    statement->row_to_insert.id       = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

/*
 * prepare_statement()
 */
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement)
{
    if(strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        return prepare_insert(input_buffer, statement);
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
    void*    node;
    uint32_t num_cells;

    node      = get_page(table->pager, table->root_page_num);
    num_cells = (*leaf_node_num_cells(node));

    // TODO: remove this 
    if(num_cells >= LEAF_NODE_MAX_CELLS)
    {
        return EXECUTE_TABLE_FULL;
    }
    
    Row*     row_to_insert;
    Cursor*  cursor;
    uint32_t key_to_insert;

    row_to_insert = &(statement->row_to_insert);
    key_to_insert = row_to_insert->id;
    cursor        = table_find(table, key_to_insert);
    
    // Check for duplicates
    if(cursor->cell_num < num_cells)
    {
        uint32_t key_at_index = (*leaf_node_key(node, cursor->cell_num));
        if(key_at_index == key_to_insert)
            return EXECUTE_DUPLICATE_KEY;
    }

    leaf_node_insert(
            cursor, 
            row_to_insert->id, 
            row_to_insert
    );

    free(cursor);

    return EXECUTE_SUCCESS;
}

/*
 * execute_select()
 */
ExecuteResult execute_select(Statement* statement, Table* table)
{
    Row     row;
    Cursor* cursor;

    cursor = table_start(table);
    while(!(cursor->end_of_table))
    {
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }

    free(cursor);

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

