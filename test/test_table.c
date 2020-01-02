/*
 * TEST_TABLE
 * Unit tests for Table object 
 *
 * Stefan Wong 2019
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>

// units under test
#include "input.h"
#include "table.h"


/*
 * Basic init test
 */
START_TEST(test_create_table)
{
    Table* table;

    table = new_table();
    ck_assert_ptr_ne(NULL, table);

    free_table(table);

} END_TEST


/*
 * Test commands
 */
START_TEST(test_insert_table)
{
    char inp_valid[]    = "insert 1 user1 user1@domain.net";
    char exp_username[] = "user1";
    char exp_email[]    = "user1@domain.net";

    Statement statement;
    PrepareResult prep_result;
    ExecuteResult exec_result;
    Table*        table;
    InputBuffer*  input_buffer;

    // get a new table
    table = new_table();
    ck_assert_ptr_ne(NULL, table);
    ck_assert_int_eq(0, table->num_rows);

    // get a new input buffer
    input_buffer = new_input_buffer();
    ck_assert_ptr_ne(NULL, input_buffer);

    input_buffer->buffer = inp_valid;
    fprintf(stdout, "[%s] checking table operation with input \n\t[%s]\n",
            __func__, input_buffer->buffer
    );

    // Insert some data into the table
    prep_result = prepare_statement(input_buffer, &statement);
    ck_assert_int_eq(PREPARE_SUCCESS, prep_result);

    exec_result = execute_statement(&statement, table);
    ck_assert_int_eq(EXECUTE_SUCCESS, exec_result);

    // Check the data 
    ck_assert_int_eq(1, table->num_rows);
    // If we print the data here we need to capture stdout and compare
    // strings. We can alternatively deserialize the rows manually
    // and compare the structure contents.

    Row row;
    deserialize_row(row_slot(table, 0), &row);
    
    ck_assert_int_eq(1, row.id);
    // expecting username to  be 'user1'  (5 chars)
    for(int c = 0; c < 5; ++c)
        ck_assert_int_eq(exp_username[c], row.username[c]);
    // expecting email to be user1@domain.net (17 chars)
    for(int c = 0; c < 17; ++c)
        ck_assert_int_eq(exp_email[c], row.email[c]);

    //close_input_buffer(input_buffer);
    free_table(table);

} END_TEST


/*
 * Test table limit
 */
// At this time, the maximum number of pages in a single table is 100
START_TEST(test_fill_table)
{
    char input[255];
    Table* table;
    Statement     statement;
    PrepareResult prep_result;
    ExecuteResult exec_result;
    InputBuffer*  input_buffer;

    // Get a table
    table = new_table();
    ck_assert_ptr_ne(NULL, table);
    ck_assert_int_eq(0, table->num_rows);

    // Get a buffer for commands
    input_buffer = new_input_buffer();
    ck_assert_ptr_ne(NULL, input_buffer);

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
        ck_assert_int_eq(PREPARE_SUCCESS, prep_result);
        exec_result = execute_statement(&statement, table);

        // Check the expected exec_result here 
        if(cur_row < exp_max_row)
            ck_assert_int_eq(EXECUTE_SUCCESS, exec_result);
        else
            ck_assert_int_eq(EXECUTE_TABLE_FULL, exec_result);

    } while(exec_result != EXECUTE_TABLE_FULL);

    fprintf(stdout, "[%s] inserted %d rows into table\n", __func__, cur_row);
    ck_assert_int_eq(1300, table->num_rows);
    ck_assert_int_eq(table->max_rows, table->num_rows);

    // Any subsequent inserts should fail
    for(int i = 0; i < 128; ++i)
    {
        sprintf(input, "insert %d user%d, email%d@domain.net", i+cur_row, i+cur_row, i+cur_row);
        input_buffer->buffer = input;
        prep_result = prepare_statement(input_buffer, &statement);
        ck_assert_int_eq(PREPARE_SUCCESS, prep_result);
        exec_result = execute_statement(&statement, table);
        ck_assert_int_eq(EXECUTE_TABLE_FULL, exec_result);
    }

    close_input_buffer(input_buffer);
    //free_table(table);

}END_TEST


/*
 * Test character limit 
 */
START_TEST(test_char_limit)
{
    char input[798];        // this size prevents sprintf destinaton size warning
    char long_name[256];
    char long_email[256];

    Table* table;
    Statement     statement;
    PrepareResult prep_result;
    InputBuffer*  input_buffer;

    table = new_table();
    ck_assert_ptr_ne(NULL, table);
    ck_assert_int_eq(0, table->num_rows);

    // Get a buffer for commands
    input_buffer = new_input_buffer();
    ck_assert_ptr_ne(NULL, input_buffer);

    // Create a long user name
    long_name[0] = 'u';
    long_name[0] = 's';
    long_name[0] = 'e';
    long_name[0] = 'r';
    for(int c = 5; c < 128; ++c)
        long_name[c] = '0';
    long_name[129] = '\0';

    // Create a long email
    //sprintf(long_email, "user");
    long_email[0] = 'u';
    long_email[1] = 's';
    long_email[2] = 'e';
    long_email[3] = 'r';
    for(int c = 4; c < 150; ++c)
        long_email[c] = '0';

    long_email[151] = '@'; 
    long_email[152] = 'd';
    long_email[153] = '.';
    long_email[154] = 'n';
    long_email[155] = 'e';
    long_email[156] = 't';
    long_email[157] = '\0';

    // Now try to insert into table
    sprintf(input, "insert %d %s %s", 44, long_name, long_email);
    input_buffer->buffer = input;

    // Insert some data into the table
    prep_result = prepare_statement(input_buffer, &statement);
    ck_assert_int_eq(PREPARE_STRING_TOO_LONG, prep_result);

    //close_input_buffer(input_buffer);
    free_table(table);

} END_TEST


/*
 * Create test suite
 */
Suite* sq_table_suite(void)
{
    Suite* s;
    TCase* table_create;
    TCase* table_insert;
    TCase* table_fill;
    TCase* table_char_limit;

    s = suite_create("sqlite_table");
    // Create test 
    table_create = tcase_create("Create Table");
    tcase_add_test(table_create, test_create_table);
    suite_add_tcase(s, table_create);
    // Test the commands work
    table_insert = tcase_create("Insert Table");
    tcase_add_test(table_insert, test_insert_table);
    suite_add_tcase(s, table_insert);
    // Table fill test
    table_fill = tcase_create("Fill Table");
    tcase_add_test(table_fill, test_fill_table);
    suite_add_tcase(s, table_fill);
    // Test character limit
    table_char_limit = tcase_create("Table Character Limit");
    tcase_add_test(table_char_limit, test_char_limit);
    suite_add_tcase(s, table_char_limit);

    return s;
}


int main(void)
{
    int num_failed;
    Suite* s;
    SRunner* sr;

    s = sq_table_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    num_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (num_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
