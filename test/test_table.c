/*
 * TEST_TABLE
 * Unit tests for Table object 
 *
 * Stefan Wong 2019
 */

#include <stdlib.h>
#include <check.h>
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
 * Create test suite
 */
Suite* sq_table_suite(void)
{
    Suite* s;
    TCase* table_create;

    s = suite_create("sqlite_table");
    // Create test 
    table_create = tcase_create("Create Table");
    tcase_add_test(table_create, test_create_table);
    suite_add_tcase(s, table_create);

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
