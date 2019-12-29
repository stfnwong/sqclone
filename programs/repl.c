/*
 * REPL
 * Implementation of an SQLite-like REPL
 * I'm mainly following along from (https://cstack.github.io/db_tutorial/parts/part1.html), which is where the real credit should go.
 *
 * Stefan Wong 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Metacommand stuff 
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum 
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;


/*
 * do_meta_command()
 * Handle metacommands here
 */
MetaCommandResult do_meta_command(InputBuffer* input_buffer)
{
    if(strncmp(input_buffer->buffer, ".exit", 6) == 0)
    {
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
} Statement;

/*
 * prepare_statement()
 */
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement)
{
    if(strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        statement->type = STATEMENT_INSERT;
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
void execute_statement(Statement* statement)
{
    switch(statement->type)
    {
        case STATEMENT_INSERT:
            fprintf(stdout, "[%s] here is where we do an INSERT\n", __func__);
            break;

        case STATEMENT_SELECT:
            fprintf(stdout, "[%s] here is where we do a SELECT\n", __func__);
            break;
    }

    // So far, nothing could go wrong here so there is no error handling. 
    // As the implementation becomes more complete there will need to be 
    // additional logic here to take care of possible errors.
}



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


/*
 * close_input_buffer()
 * Close a buffer and free associated memory
 */
void close_input_buffer(InputBuffer* input_buffer)
{
    free(input_buffer->buffer);
    free(input_buffer);
}


// Entry point
int main(int argc, char *argv[])
{
    InputBuffer* input_buffer = new_input_buffer();

    while(1)
    {
        print_prompt();
        read_input(input_buffer);

        // Commands start with '.' character
        if(input_buffer->buffer[0] == '.')
        {
            switch(do_meta_command(input_buffer))
            {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    fprintf(stdout, "Unrecognized command [%s].\n", input_buffer->buffer);
                    continue;
            }
        }

        Statement statement;

        switch(prepare_statement(input_buffer, &statement))
        {
            case PREPARE_SUCCESS:
                break;
            case PREPARE_UNRECOGNIZED_STATEMENT:
                fprintf(stdout, "Unrecognized keyword at start of [%s]\n",
                        input_buffer->buffer
                );
                continue;
        }

        execute_statement(&statement);
        fprintf(stdout, "Executed.\n");
    }

    return 0;
}
