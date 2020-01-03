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
    if(table->num_rows >= table->max_rows)
    {
        return EXECUTE_TABLE_FULL;
    }
    
    Row*    row_to_insert;
    Cursor* cursor;

    cursor        = table_end(table);
    row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, cursor_value(cursor));
    table->num_rows++;

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

