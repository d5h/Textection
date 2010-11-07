#include <cassert>
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

typedef std::map<int, std::vector<double> > char_result_map_t;

struct FeatureData {
  Feature *feature;
  char_result_map_t char_results;
  std::vector<double> junk_results;

  FeatureData(Feature *f)
    : feature(f)
  { }
};

struct process_data_t {
  sqlite3 *db;
  std::vector<FeatureData> features;
};

static void
run_feature(FeatureData &data, const obj_desc_set_t &descriptors,
            objs_t &objs, size_t subject)
{
  double result = data.feature->describe(objs, subject);

  const Obj &obj = objs[subject];
  obj_desc_t key;
  key.x = obj.runs[0].start;
  key.y = obj.runs[0].row;
  obj_desc_set_t::iterator desc_iter = descriptors.find(key);

  if (desc_iter == descriptors.end())
    data.junk_results.push_back(result);
  else {
    int c = desc_iter->c;
    data.char_results[c].push_back(result);
  }
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

  for (size_t f = 0; f < data->features.size(); ++f) {
    for (size_t j = 0; j < objs.size(); ++j)
      run_feature(data->features[f], descriptors, objs, j);
  }
}

struct stats_t {
  double ave, dev;
};

static stats_t
calc_stats(const std::vector<double> &x)
{
  stats_t stats;
  double n = x.size();

  stats.ave = 0.;
  for (size_t i = 0; i < x.size(); ++i)
    stats.ave += x[i] / n;

  if (n == 1.)
    stats.dev = -1.;
  else {
    stats.dev = 0.;
    for (size_t i = 0; i < x.size(); ++i) {
      double xd = x[i] - stats.ave;
      xd *= xd;
      stats.dev += xd / (n - 1);
    }
  }

  return stats;
}

static void
compile_stats(const std::vector<FeatureData> &data)
{
  for (size_t i = 0; i < data.size(); ++i) {
    const FeatureData &f = data[i];
    std::cout << f.feature->name() << ":\n";
    char_result_map_t::const_iterator iter;
    for (iter = f.char_results.begin(); iter != f.char_results.end(); ++iter) {
      char c = iter->first;
      stats_t stats = calc_stats(iter->second);
      std::cout << "'" << c << "': "
                << "average=" << stats.ave << "; "
                << "std dev=" << stats.dev << "\n";
    }
  }
}

int
main()
{
  sqlite3 *db;
  SQL_OK(open_sql_db_and_ensure_close_on_exit("objs.sqlite", &db));

  process_data_t proc_data;
  proc_data.db = db;
  proc_data.features.push_back(FeatureData(new AspectRatioFeature()));
  proc_data.features.push_back(FeatureData(new TopPositionFeature()));
  proc_data.features.push_back(FeatureData(new BottomPositionFeature()));

  for_each_table(db, process_table, &proc_data);
  compile_stats(proc_data.features);

  return 0;
}
