#include <cassert>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "features.h"
#include "image.h"
#include "sql.h"

const std::ios_base::openmode SS_BUFFER_MODE =
  std::stringstream::ate | std::stringstream::out;

typedef void (*table_processor_t)(const std::string &, void *);

struct callback_data_t {
  table_processor_t proc;
  void *proc_data;
};

static int
for_each_table_callback(void *ptr, int ncolumns, char **text, char **names)
{
  callback_data_t *data = static_cast<callback_data_t *>(ptr);
  std::string name(text[0]);
  data->proc(name, data->proc_data);
  return 0;
}

static void
for_each_table(sqlite3 *db, table_processor_t proc, void *proc_data)
{
  const char *stmt = "SELECT name FROM sqlite_master WHERE type='table'";
  callback_data_t data;
  data.proc = proc;
  data.proc_data = proc_data;
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

static bool operator <(const obj_desc_t &o1, const obj_desc_t &o2)
{
  return o1.y < o2.y || (o1.y == o2.y && o1.x < o2.x);
}

typedef std::set<obj_desc_t> obj_desc_set_t;

static int
load_descriptors_callback(void *ptr, int ncolumns, char **text, char **names)
{
  obj_desc_set_t *descriptors = static_cast<obj_desc_set_t *>(ptr);
  assert(ncolumns == 3);
  obj_desc_t desc;
  desc.x = boost::lexical_cast<int>(text[0]);
  desc.y = boost::lexical_cast<int>(text[1]);
  desc.c = boost::lexical_cast<int>(text[2]);
  descriptors->insert(desc);
  return 0;
}

static void
load_object_descriptors(const std::string &table_name,
                        sqlite3 *db,
                        obj_desc_set_t &descriptors)
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

struct process_data_t {
  sqlite3 *db;
  std::vector<Feature *> features;
};

static void
output_data(objs_t &objs, size_t subject,
            const std::vector<Feature *> &features,
            const obj_desc_set_t &descriptors)
{
  for (size_t i = 0; i < features.size(); ++i) {
    double result = features[i]->describe(objs, subject);
    std::cout << result << ",";
  }

  const Obj &obj = objs[subject];
  obj_desc_t key;
  key.x = obj.runs[0].start;
  key.y = obj.runs[0].row;
  obj_desc_set_t::iterator desc_iter = descriptors.find(key);

  if (desc_iter == descriptors.end())
    std::cout << 0 << std::endl;
  else
    std::cout << desc_iter->c << std::endl;
}

static void
process_table(const std::string &name, void *ptr)
{
  process_data_t *data = static_cast<process_data_t *>(ptr);
  std::vector<double> params;
  std::string filename = deconstruct_table_name(name, params);

  obj_desc_set_t descriptors;
  load_object_descriptors(name, data->db, descriptors);

  cv::Mat img = cv::imread(filename);
  std::vector<Obj> objs;
  get_sorted_objects_from_image(img, objs, params);

  for (size_t i = 0; i < objs.size(); ++i)
    output_data(objs, i, data->features, descriptors);
}

int
main()
{
  sqlite3 *db;
  SQL_OK(open_sql_db_and_ensure_close_on_exit("objs.sqlite", &db));

  process_data_t proc_data;
  proc_data.db = db;
  proc_data.features.push_back(new AspectRatioFeature());
  proc_data.features.push_back(new TopPositionFeature());
  proc_data.features.push_back(new BottomPositionFeature());

  std::cout << "@RELATION textection\n" << std::endl;
  for (size_t i = 0; i < proc_data.features.size(); ++i) {
    std::cout << "@ATTRIBUTE " << proc_data.features[i]->name()
              << " NUMERIC\n";
  }
  std::cout << "@ATTRIBUTE class {0";
  for (int c = 0; c < 128; ++c) {
    if (std::isgraph(c))
      std::cout << "," << c;
  }
  std::cout << "}\n\n" << "@DATA" << std::endl;

  for_each_table(db, process_table, &proc_data);

  return 0;
}
