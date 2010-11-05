#ifndef SQL_INCLUDED
#define SQL_INCLUDED 1

#include <iostream>
#include <string>

#include <sqlite3.h>

#define SQL_CHECK(code, expected)                               \
  do {                                                          \
    int sql_check_code = (code);                                \
    if (sql_check_code != (expected)) {                         \
      std::cerr << "Sqlite3 fail: " __FILE__ ":"                \
                <<  __LINE__ << ": "                            \
                << sql_check_code << std::endl;                 \
      std::exit(1);                                             \
    }                                                           \
  } while (0)

#define SQL_OK(code) SQL_CHECK(code, SQLITE_OK)

extern int
open_sql_db_and_ensure_close_on_exit(const char *filename,
                                     sqlite3 **db);

#endif  // SQL_INCLUDED
