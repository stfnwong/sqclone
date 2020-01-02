/*
 * TABLE_SPEC
 * BDD test for Table object
 *
 * Stefan Wong 2020
 */

#include <string.h>
#include <stdio.h>

// units under test 
#include "input.h"
#include "table.h"
// testing framework
#include "bdd-for-c.h"


spec("table")
{
    // Check that we can create a table object
    it("creates a non null pointer when initalized")
    {
        Table* table;

        table = new_table();
        check(table != NULL);
        free_table(table);
    }

    // Test row insertion
    it("inserts and selects a row")
    {
        char inp_valid[]    = "insert 1 user1 user1@domain.net";
        char exp_username[] = "user1";
        char exp_email[]    = "user1@domain.net";

        Statement statement;
        PrepareResult prep_result;
        ExecuteResult exec_result;
        Table*        table;
        InputBuffer*  input_buffer;

        // Get an input buffer
        input_buffer = new_input_buffer();
        check(input_buffer != NULL);

        // get a new table
        table = new_table();
        check(table != NULL);
        check(table->num_rows == 0);
        input_buffer->buffer = inp_valid;
        fprintf(stdout, "[%s] checking table operation with input \n\t[%s]\n",
                __func__, input_buffer->buffer
        );

        // Insert some data into the table
        prep_result = prepare_statement(input_buffer, &statement);
        check(prep_result == PREPARE_SUCCESS);
        exec_result = execute_statement(&statement, table);
        check(exec_result == EXECUTE_SUCCESS);

        // Check the data 
        check(table->num_rows == 1);
        Row row;
        deserialize_row(row_slot(table, 0), &row);
        
        check(row.id == 1);
        // expecting username to  be 'user1'  (5 chars)
        for(int c = 0; c < 5; ++c)
            check(exp_username[c] == row.username[c]);
        // expecting email to be user1@domain.net (17 chars)
        for(int c = 0; c < 17; ++c)
            check(exp_email[c] == row.email[c]);

        //close_input_buffer(input_buffer);
        free_table(table);
    }

    it("returns EXECUTE_TABLE_FULL when full")
    {
        char*         input;
        Table*        table;
        Statement     statement;
        PrepareResult prep_result;
        ExecuteResult exec_result;
        InputBuffer*  input_buffer;

        input = malloc(sizeof(char) * 256);
        check(input != NULL);

        // Get a table
        table = new_table();
        check(table != NULL);
        check(table->num_rows == 0);

        // Get a buffer for commands
        input_buffer = new_input_buffer();
        check(input_buffer != NULL);

        print_page_info();
        fprintf(stdout, "\n");

        fprintf(stdout, "[%s] inserting rows into table...\n", __func__);
        // Create new rows until the table is full
        int exp_max_row = 1300;     // 13 rows per page, 100 pages per table
        int cur_row = -1;
        do
        {
            cur_row++;
            sprintf(input, "insert %d user%d, email%d@domain.net", cur_row, cur_row, cur_row);
            input_buffer->buffer = input;
            prep_result = prepare_statement(input_buffer, &statement);
            check(prep_result == PREPARE_SUCCESS);
            exec_result = execute_statement(&statement, table);

            // Check the expected exec_result here 
            if(cur_row < exp_max_row)
            {
                it("should return success")
                    check(exec_result == EXECUTE_SUCCESS);
            }
            else
            {
                it("should return full")
                    check(exec_result == EXECUTE_TABLE_FULL);
            }

        } while(exec_result != EXECUTE_TABLE_FULL);

        fprintf(stdout, "[%s] inserted %d rows into table\n", __func__, cur_row);
        check(table->num_rows == 1300);
        check(table->max_rows == table->num_rows);

        // Any subsequent inserts should fail
        for(int i = 0; i < 128; ++i)
        {
            sprintf(input, "insert %d user%d, email%d@domain.net", i+cur_row, i+cur_row, i+cur_row);
            input_buffer->buffer = input;
            prep_result = prepare_statement(input_buffer, &statement);
            check(prep_result == PREPARE_SUCCESS);
            exec_result = execute_statement(&statement, table);
            check(exec_result == EXECUTE_TABLE_FULL);
        }

        close_input_buffer(input_buffer);
        //free_table(table);        // TODO: can't free table?
    }
}
