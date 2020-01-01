/*
 * INPUT
 * Input handling functions 
 *
 * Stefan Wong 2019
 */

#ifndef __SQ_INPUT_H
#define __SQ_INPUT_H

#include "table.h"

// Input buffer structure
typedef struct 
{
    char*   buffer;
    size_t  buffer_length;
    ssize_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer(void);
void         close_input_buffer(InputBuffer* input_buffer);
void         read_input(InputBuffer* input_buffer);

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

// Metacommand stuff 
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table);

// Output of command preparation
typedef enum 
{
    PREPARE_SUCCESS,
    PREPARE_STRING_TOO_LONG,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);


// Execution stuff
typedef enum 
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_select(Statement* statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);

#endif /*__SQ_INPUT_H*/
