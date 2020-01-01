/*
 * INPUT
 * Input handling functions 
 *
 * Stefan Wong 2019
 */

#include <stdlib.h>
#include <stdio.h>
#include "input.h"

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
        close_input_buffer(input_buffer);
        free_table(table);
        exit(EXIT_SUCCESS);
    }
    else
        return META_COMMAND_UNRECOGNIZED_COMMAND;
}


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
    if(table->num_rows >= table->max_rows)
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
