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

// Methods for working with an InputBuffer

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

        if(strcmp(input_buffer->buffer, ".exit") == 0)
        {
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stdout, "Unrecognized command [%s].\n", input_buffer->buffer);
        }

    }

    return 0;
}
