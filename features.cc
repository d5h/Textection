#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "image.h"
#include "sql.h"

const std::ios_base::openmode SS_BUFFER_MODE =
  std::stringstream::ate | std::stringstream::out;

typedef void (*table_processor_t)(const std::string &, sqlite3 *);

struct callback_data_t {
  table_processor_t proc;
  sqlite3 *db;
};

static int
for_each_table_callback(void *ptr, int ncolumns, char **text, char **names)
{
  callback_data_t *data = static_cast<callback_data_t *>(ptr);
  std::string name(text[0]);
  data->proc(name, data->db);
  return 0;
}

static void
for_each_table(sqlite3 *db, table_processor_t proc)
{
  const char *stmt = "SELECT name FROM sqlite_master WHERE type='table'";
  callback_data_t data;
  data.proc = proc;
  data.db = db;
  SQL_OK(sqlite3_exec(db, stmt, for_each_table_callback, &data, 0));
}

static std::string
deconstruct_table_name(const std::string &table_name,
                       std::vector<double> &params)
{
  size_t plus_index = table_name.find_last_of('+');
  assert(plus_index != std::string::npos);
  std::string filename = table_name.substr(0, plus_index);

  size_t end_num;
  // Expect at least one parameter
  size_t start_num = plus_index + 1;
  do {
    end_num = table_name.find(',', start_num);
    std::string num_str = table_name.substr(start_num, end_num);
    double num = boost::lexical_cast<double>(num_str);
    params.push_back(num);
    start_num = end_num + 1;
  } while (end_num != std::string::npos);

  return filename;
}

struct obj_desc_t {
  int x, y, c;
};

typedef std::vector<obj_desc_t> obj_desc_vec_t;

static int
load_descriptors_callback(void *ptr, int ncolumns, char **text, char **names)
{
  obj_desc_vec_t *descriptors = static_cast<obj_desc_vec_t *>(ptr);
  assert(ncolumns == 3);
  obj_desc_t desc;
  desc.x = boost::lexical_cast<int>(text[0]);
  desc.y = boost::lexical_cast<int>(text[1]);
  desc.c = boost::lexical_cast<int>(text[2]);
  descriptors->push_back(desc);
  return 0;
}

static void
load_object_descriptors(const std::string &table_name,
                        sqlite3 *db,
                        obj_desc_vec_t &descriptors)
{
  std::stringstream buffer(
    "SELECT x, y, char FROM '",
    SS_BUFFER_MODE
    );
  buffer << table_name << "';";
  std::string stmt_str = buffer.str();
  const char *stmt = stmt_str.c_str();

  SQL_OK(sqlite3_exec(db, stmt, load_descriptors_callback, &descriptors, 0));
}

static void
process_table(const std::string &name, sqlite3 *db)
{
  std::vector<double> params;
  std::string filename = deconstruct_table_name(name, params);

  obj_desc_vec_t descriptors;
  load_object_descriptors(name, db, descriptors);

  cv::Mat img = cv::imread(filename);
  std::vector<Obj> objs;
  get_sorted_objects_from_image(img, objs, params);
}

int
main()
{
  sqlite3 *db;
  SQL_OK(open_sql_db_and_ensure_close_on_exit("objs.sqlite", &db));

  for_each_table(db, process_table);
  return 0;
}
