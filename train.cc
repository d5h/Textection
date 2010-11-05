#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <sqlite3.h>

#include "keycode.h"
#include "objfind.h"

std::string window_name = "Textection training";
sqlite3 *db;

const std::ios_base::openmode SS_BUFFER_MODE =
  std::stringstream::ate | std::stringstream::out;

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

static void
close_db(void)
{
  if (db)
    sqlite3_close(db);
}

static int
get_key(void)
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

static void
parse_args(char **argv, std::vector<double> &params, std::vector<std::string> &args)
{
  size_t i = 0;
  const size_t NOWARN = std::numeric_limits<size_t>::max();

  for ( ; *argv; ++argv) {
    const char *s = *argv;
    bool num = true;
    for ( ; *s; ++s) {
      if (! (isdigit(*s) || *s == '.')) {
        num = false;
        break;
      }
    }
    if (num) {
      if (i < params.size())
        params[i] = boost::lexical_cast<double>(*argv);
      else if (i != NOWARN) {
        i = NOWARN;
        std::cerr << "Too many numerical parameters" << std::endl;
      }
    } else
      args.push_back(*argv);
  }
}

static void
truncColors(cv::Mat &m, int ncolors)
{
  assert(m.type() == CV_8UC1);
  uchar n = 256 / ncolors;

  cv::Size size = m.size();
  if (m.isContinuous()) {
    size.width *= size.height;
    size.height = 1;
  }

  for (int i = 0; i < size.height; ++i) {
    uchar *p = m.ptr(i);
    for (int j = 0; j < size.width; ++j) {
      p[j] = (p[j] / n) * n;
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
  cv::Mat final, gray, pyrd, pyru;
  cv::pyrDown(img, pyrd);
  cv::pyrUp(pyrd, pyru);
  cv::cvtColor(pyru, gray, CV_BGR2GRAY);
  cv::equalizeHist(gray, final);
  truncColors(final, params[0]);

  std::vector<Obj> objs;
  objfind(final, objs);
  sortobjs(objs);

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
  int status = sqlite3_open("objs.sqlite", &db);
  std::atexit(close_db);
  SQL_OK(status);

  cv::namedWindow(window_name, CV_WINDOW_AUTOSIZE);

  std::vector<std::string> args;
  std::vector<double> params;
  params.push_back(8);
  parse_args(argv + 1, params, args);

  for (size_t n = 0; n < args.size(); ++n) {
    std::cout << args[n] << std::endl;
    process_img(args[n], params, db);
  }

  return 0;
}
