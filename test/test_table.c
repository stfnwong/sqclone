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
} END_TEST


/*
 * Test commands
 */
START_TEST(test_command_table)
{
    char inp_valid[]    = "insert 1 user1 user1@domain.net";
    char exp_username[] = "user1";
    char exp_email[]    = "user1@domain.net";

    Statement statement;
    PrepareResult prep_result;
    ExecuteResult exec_result;
    Table* table;
    InputBuffer* input_buffer;

    table = new_table();
    ck_assert_ptr_ne(NULL, table);
    ck_assert_int_eq(0, table->num_rows);

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


} END_TEST


/*
 * Test table limit
 */
START_TEST(test_fill_table)
{
    char input[255];
    Table* table;

    table = new_table();
    ck_assert_ptr_ne(NULL, table);
}END_TEST


/*
 * Create test suite
 */
Suite* sq_table_suite(void)
{
    Suite* s;
    TCase* table_create;
    TCase* table_command;
    TCase* table_fill;

    s = suite_create("sqlite_table");
    // Create test 
    table_create = tcase_create("Create Table");
    tcase_add_test(table_create, test_create_table);
    suite_add_tcase(s, table_create);
    // Test the commands work
    table_command = tcase_create("Command Table");
    tcase_add_test(table_command, test_command_table);
    suite_add_tcase(s, table_command);
    // Table fill test
    table_fill = tcase_create("Fill Table");
    tcase_add_test(table_fill, test_fill_table);
    suite_add_tcase(s, table_fill);

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
