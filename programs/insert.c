/*
 * INSERT
 * Program that inserts a record into the DB. This is just to make 
 * debugging the insert simpler, its useless otherwise.
 *
 * Stefan Wong 2020
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "input.h"
#include "table.h"


// TODO: factor this out into input.h

int main(int argc, char *argv[])
{
    char* filename;
    InputBuffer* input_buffer = new_input_buffer();
    Statement statement;
    Table* table;

    // For now we just accept a single argument - the name of the db file
    if(argc < 2)
    {
        fprintf(stderr, "No database name specified\n");
        exit(EXIT_FAILURE);
    }

    filename = argv[1];
    table = db_open(filename);
    if(!table)
    {
        fprintf(stderr, "[%s] failed to allocate memory for table\n", __func__);
        exit(EXIT_FAILURE);
    }

    // Now do a single insert command
    PrepareResult prep_result;
    ExecuteResult exec_result;
    const char* command  = "insert 1 test test@fake.net";

    // malloc this so that the when the buffer is cleaned up we 
    // dont get a double free.
    char* command_mem = malloc(sizeof(char) * strlen(command) + 1);

    strcpy(command_mem, command); 
    input_buffer->buffer = command_mem;

    prep_result = prepare_statement(input_buffer, &statement);
    if(prep_result != PREPARE_SUCCESS)
    {
        fprintf(stdout, "[%s] failed to prepare input command [%s]\n",
               __func__, input_buffer->buffer);
        exit(EXIT_FAILURE);
    }

    exec_result = execute_statement(&statement, table); 
    if(exec_result != EXECUTE_SUCCESS)
    {
        fprintf(stdout, "[%s] failed to execute input command [%s]\n",
               __func__, input_buffer->buffer);
        exit(EXIT_FAILURE);
    }

    // May as well do a select while we are here
    Row row;
    Cursor* cursor = table_start(table);
    deserialize_row(cursor_value(cursor), &row);
    print_row(&row);

    // Close the db - we expect the data to be flushed to disk here.
    db_close(table);
    close_input_buffer(input_buffer);

    return 0;
}
