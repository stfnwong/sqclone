/*
 * TABLE_SPEC
 * BDD test for Table object
 *
 * Stefan Wong 2020
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>     // for access()

// units under test 
#include "input.h"
#include "table.h"
// testing framework
#include "bdd-for-c.h"


spec("table")
{
    static const char* test_db_name = "test/test.db";

    after_each()
    {
        fprintf(stdout, "[%s] removing db file [%s]\n", __func__, test_db_name);
        int status = remove(test_db_name);
        if(status != 0)
            fprintf(stderr, "[%s] failed to remove db file [%s]\n", __func__, test_db_name);
    }

    // Check that we can create a table object
    it("creates a non null pointer when initalized")
    {
        Table* table;

        table = db_open(test_db_name);
        check(table != NULL);
        db_close(table);
    }

    // Test row insertion
    it("inserts and selects a row")
    {
        char       inp_valid[]    = "insert 1 user1 user1@domain.net";
        char       exp_username[] = "user1";
        char       exp_email[]    = "user1@domain.net";

        Statement statement;
        PrepareResult prep_result;
        ExecuteResult exec_result;
        Table*        table;
        Cursor*       cursor;
        InputBuffer*  input_buffer;
        Row           row;

        // Get an input buffer
        input_buffer = new_input_buffer();
        check(input_buffer != NULL);

        // get a new table
        table = db_open(test_db_name);
        check(table != NULL);
        //check(table->num_rows == 0);
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
        //check(table->num_rows == 1);
        cursor = table_start(table);
        deserialize_row(cursor_value(cursor), &row);

        //fprintf(stdout, "[%s] deserialized row (%d/%d) :\n", __func__, row.id, table->num_rows);
        print_row(&row);
        
        //check(row.id == 1);
        // expecting username to  be 'user1'  (5 chars)
        for(int c = 0; c < 5; ++c)
            check(exp_username[c] == row.username[c]);
        // expecting email to be user1@domain.net (17 chars)
        for(int c = 0; c < 17; ++c)
            check(exp_email[c] == row.email[c]);

        //close_input_buffer(input_buffer);
        db_close(table);
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
        table = db_open(test_db_name);
        check(table != NULL);
        //check(table->num_rows == 0);

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
        //check(table->num_rows == 1300);
        //check(table->max_rows == table->num_rows);

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
        //db_close(table);        // TODO: can't free table?
    }

    it("rejects names longer than 255 chars")
    {
        char        long_name[300];
        char*       input;

        Table* table;
        Statement     statement;
        PrepareResult prep_result;
        InputBuffer*  input_buffer;

        input = malloc(sizeof(char) * 798);

        // Get a table
        table = db_open(test_db_name);
        check(table != NULL);
        //check(table->num_rows == 0);

        // Get a buffer for commands
        input_buffer = new_input_buffer();
        check(input_buffer != NULL);

        // Create a long user name
        long_name[0] = 'u';
        long_name[1] = 's';
        long_name[2] = 'e';
        long_name[3] = 'r';
        for(int c = 4; c < 229; ++c)
            long_name[c] = 'a';
        long_name[229] = '\0';

        sprintf(input, "insert 44 %s e@mail.net", long_name);
        input_buffer->buffer = input;
        // Insert some data into the table
        prep_result = prepare_statement(input_buffer, &statement);
        check(prep_result == PREPARE_STRING_TOO_LONG);

        free(table);
        close_input_buffer(input_buffer);
    }

    it("rejects emails longer than 255 chars")
    {
        char        long_email[300];
        char*       input;

        Table* table;
        Statement     statement;
        PrepareResult prep_result;
        InputBuffer*  input_buffer;

        input = malloc(sizeof(char) * 798);

        // Get a table
        table = db_open(test_db_name);
        check(table != NULL);
        // TODO : check number of records

        // Get a buffer for commands
        input_buffer = new_input_buffer();
        check(input_buffer != NULL);

        // Create a long email
        long_email[0] = 'u';
        long_email[1] = 's';
        long_email[2] = 'e';
        long_email[3] = 'r';
        for(int c = 4; c < 289; ++c)
            long_email[c] = '0';

        long_email[289] = '@'; 
        long_email[290] = 'd';
        long_email[291] = '.';
        long_email[292] = 'n';
        long_email[293] = 'e';
        long_email[294] = 't';
        long_email[295] = '\0';

        sprintf(input, "insert 10 user_with_long_name %s", long_email);
        input_buffer->buffer = input;
        prep_result = prepare_statement(input_buffer, &statement);
        check(prep_result == PREPARE_STRING_TOO_LONG);

        free(table);
        close_input_buffer(input_buffer);
    }

    it("rejects negative ids")
    {
        char*         input;
        Table*        table;
        Statement     statement;
        PrepareResult prep_result;
        ExecuteResult exec_result;
        InputBuffer*  input_buffer;

        // Get a new table
        table = db_open(test_db_name);
        check(table != NULL);
        // TODO : check number of records

        // Allocate input here to avoid double-free
        input = malloc(sizeof(char) * 256);
        check(input != NULL);

        // get a new input buffer
        input_buffer = new_input_buffer();
        check(input_buffer != NULL);

        // Insert some invalid data
        strcpy(input, "insert -1 user1 user1@domain.net");
        input_buffer->buffer = input;
        fprintf(stdout, "[%s] checking table operation with input \n\t[%s]\n",
                __func__, input_buffer->buffer
        );

        prep_result = prepare_statement(input_buffer, &statement);
        check(prep_result == PREPARE_NEGATIVE_ID);
        // TODO: check number of records

        // Insert some more invalid data
        strcpy(input, "insert -100 user1 user1@domain.net");
        input_buffer->buffer = input;
        fprintf(stdout, "[%s] checking table operation with input \n\t[%s]\n",
                __func__, input_buffer->buffer
        );
        prep_result = prepare_statement(input_buffer, &statement);
        check(prep_result == PREPARE_NEGATIVE_ID);
        // TODO: check number of records

        // With a valid input it should be fine.
        // Insert some more invalid data
        strcpy(input, "insert 4100 user1 user1@domain.net");
        input_buffer->buffer = input;
        fprintf(stdout, "[%s] checking table operation with input \n\t[%s]\n",
                __func__, input_buffer->buffer
        );

        exec_result = execute_statement(&statement, table);
        check(exec_result == EXECUTE_SUCCESS);

        // Check the data 
        // TODO : check there is one record here

        close_input_buffer(input_buffer);
        db_close(table);
    }
}
