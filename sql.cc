#include <cstdlib>
#include <set>

#include "sql.h"

typedef std::set<sqlite3 *> db_set_t;

static void close_dbs();

static db_set_t *
get_close_set()
{
  static db_set_t *close_set = 0;
  if (! close_set) {
    close_set = new db_set_t();
    std::atexit(close_dbs);
  }
  return close_set;
}

static void
close_dbs()
{
  db_set_t *close_set = get_close_set();
  db_set_t::iterator iter;
  for (iter = close_set->begin(); iter != close_set->end(); ++iter)
    sqlite3_close(*iter);
}

int
open_sql_db_and_ensure_close_on_exit(const char *filename,
                                     sqlite3 **db)
{
  int status = sqlite3_open(filename, db);
  if (! *db)
    std::abort();

  db_set_t *close_set = get_close_set();
  close_set->insert(*db);
  return status;
}
