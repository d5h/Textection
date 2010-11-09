#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "args.h"
#include "image.h"
#include "keycode.h"
#include "sql.h"

std::string window_name = "Textection training";

const std::ios_base::openmode SS_BUFFER_MODE =
  std::stringstream::ate | std::stringstream::out;

static int
get_key()
{
  int key = cv::waitKey();
  if (key_code(key) == ESC_CODE)
    std::exit(0);
  return key;
}

static int
show(const cv::Mat &img)
{
  cv::imshow(window_name, img);
  return get_key();
}

static void
insert_obj(const Obj &obj, int code, sqlite3 *db,
           const std::string &table_name)
{
  std::stringstream buffer("INSERT INTO '", SS_BUFFER_MODE);
  buffer << table_name << "' (x, y, char) VALUES ("
         << obj.runs[0].start << ", "
         << obj.runs[0].row << ", "
         << code << ");";
  std::string stmt = buffer.str();
  SQL_OK(sqlite3_exec(db, stmt.c_str(), 0, 0, 0));
}

static bool
feedback(const cv::Mat &img, const Obj &obj, sqlite3 *db,
         const std::string &table_name)
{
  int key = show(img);
  int code = key_ascii(key);
  for ( ; ; code = key_ascii(get_key())) {
    if (code == SPACE_CODE)
      return false;
    if (code == TAB_CODE)
      return true;
    if (code < 128 && isprint(code)) {
      insert_obj(obj, code, db, table_name);
      return false;
    }
  }
}

static std::string
construct_table_name(const std::string &filename,
                     const std::vector<double> &params)
{
  std::stringstream buffer(filename, SS_BUFFER_MODE);
  for (size_t i = 0; i < params.size(); ++i) {
    buffer << ((i == 0) ? '+' : ',');
    buffer << params[i];
  }
  return buffer.str();
}

static bool
table_exists(const std::string &name, sqlite3 *db)
{
  bool exists = false;
  std::stringstream buffer(
    "SELECT name FROM sqlite_master WHERE type='table' AND name='",
    SS_BUFFER_MODE
    );
  buffer << name << "';";
  std::string stmt = buffer.str();

  sqlite3_stmt *bytecode;
  SQL_OK(sqlite3_prepare_v2(db, stmt.data(), stmt.size(), &bytecode, 0));

  int res = sqlite3_step(bytecode);
  if (res == SQLITE_ROW)
    exists = true;
  else
    SQL_CHECK(res, SQLITE_DONE);

  SQL_OK(sqlite3_finalize(bytecode));
  return exists;
}

static void
create_object_table(const std::string &name, sqlite3 *db)
{
  std::stringstream buffer("CREATE TABLE '", SS_BUFFER_MODE);
  buffer << name << "' (x INTEGER, y INTEGER, char INTEGER)";
  std::string stmt = buffer.str();
  SQL_OK(sqlite3_exec(db, stmt.c_str(), 0, 0, 0));
}

static void
process_img(const std::string &name, const std::vector<double> &params,
            sqlite3 *db)
{
  std::string table_name = construct_table_name(name, params);
  if (table_exists(table_name, db))
    return;

  create_object_table(table_name, db);

  cv::Mat img = cv::imread(name);
  std::vector<Obj> objs;
  get_sorted_objects_from_image(img, objs, params);

  bool skip = false;
  for (size_t i = 0; i < objs.size() && !skip; ++i) {
    cv::Mat m = img.clone();
    fillobj(m, objs[i], cv::Scalar(255, 255, 0));
    skip = feedback(m, objs[i], db, table_name);
  }
}

int
main(int argc, char **argv)
{
  sqlite3 *db;
  SQL_OK(open_sql_db_and_ensure_close_on_exit("objs.sqlite", &db));

  cv::namedWindow(window_name, CV_WINDOW_AUTOSIZE);

  std::vector<std::string> args;
  std::vector<double> params;
  params.push_back(ARG_DEFAULT_1);
  parse_args(argv + 1, params, args);

  for (size_t n = 0; n < args.size(); ++n)
    process_img(args[n], params, db);

  return 0;
}
