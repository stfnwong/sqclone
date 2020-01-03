/*
 * REPL
 * Implementation of an SQLite-like REPL
 * I'm mainly following along from (https://cstack.github.io/db_tutorial/parts/part1.html), which is where the real credit should go.
 *
 * Stefan Wong 2019
 */


#include <string.h>     // for memcpy()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "input.h"
#include "table.h"


/*
 * print_prompt()
 */
void print_prompt(void)
{
    fprintf(stdout, "db > ");
}


// Entry point
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

            case PREPARE_NEGATIVE_ID:
                fprintf(stdout, "Illegal ID in input [%s], ID must be positive", input_buffer->buffer);
                continue;

            case PREPARE_SYNTAX_ERROR:
                fprintf(stdout, "Syntax error in statement [%s]\n", input_buffer->buffer);
                continue;

            case PREPARE_STRING_TOO_LONG:
                fprintf(stdout, "String too long (%ld chars)\n", strlen(input_buffer->buffer));
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

    db_close(table);
    close_input_buffer(input_buffer);

    return 0;
}
