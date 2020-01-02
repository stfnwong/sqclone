/*
 * TABLE_SPEC
 * BDD test for Table object
 *
 * Stefan Wong 2020
 */

#include "input.h"
#include "table.h"
#include "bdd-for-c.h"


spec("table")
{
    it("creates a non null pointer when initalized")
    {
        Table* table;

        table = new_table();
        check(table != NULL);
        free_table(table);
    }


    //it("inserts and selects a row")
    //{
    //    check(
    //}

}
