# SQLITE CLONE 
This is a clone of SQLite3 in C. I am following along from here (https://cstack.github.io/db_tutorial/).

[![Build Status](https://travis-ci.org/stfnwong/sqclone.svg?branch=master)](https://travis-ci.org/stfnwong/sqclone)

# Future work
While using `void*` blobs and having offsets is surely quite fast, it does make some aspects of debugging and reasoning a bit awkward. Once all the main parts are in place it would be good to write a version where the nodes in the B+trees are typed and compare that to the `void*` + offset implementation here.


# Requirements 
- Unit tests use [bdd-for-c](https://github.com/grassator/bdd-for-c). The header file is included in the test directory. `bdd-for-c` requires libncurses 5.x and libbsd.
