#include <algorithm>
#include <cassert>
#include <limits>
#include <list>
#include <vector>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>

#include "objfind.h"

//namespace objfind {

struct RunNode : public RunBase {
  std::vector<size_t> adjacent;
  size_t group;
  unsigned int flags;
  int color;
};

static size_t
connect_run_to_graph(RunBase &run, int color,
                     std::vector<RunNode> &graph,
                     const std::vector<size_t> &last_row_objs)
{
  graph.push_back(RunNode());
  RunNode &node = graph.back();
  node.row = run.row;
  node.start = run.start;
  node.end = run.end;
  node.color = color;
  node.flags = 0;

  for (size_t i = 0; i < last_row_objs.size(); ++i) {
    RunNode &other = graph[last_row_objs[i]];
    assert(other.row == node.row - 1);
    if (other.color != color)
      continue;
    if (std::max(node.start, other.start) < std::min(node.end, other.end))
      node.adjacent.push_back(last_row_objs[i]);
  }

  return graph.size() - 1;
}

static void
objfind_generate_run_graph(const cv::Mat &img,
                           std::vector<RunNode> &graph)
{
  assert(img.type() == CV_8UC1);

  std::vector<size_t> last_row_objs;
  std::vector<size_t> cur_row_objs;

  for (int y = 0; y < img.rows; ++y) {
    const uchar *row = img.ptr(y);
    RunBase run;
    run.row = y;
    run.start = 0;
    int color = row[0];

    for (int x = 0; x < img.cols; ++x) {
      int xcolor = row[x];
      if (xcolor != color) {
        run.end = x;

        size_t id = connect_run_to_graph(run, color, graph, last_row_objs);
        cur_row_objs.push_back(id);

        run.start = x;
        color = xcolor;
      }
    }
    run.end = img.cols;

    size_t id = connect_run_to_graph(run, color, graph, last_row_objs);
    cur_row_objs.push_back(id);

    last_row_objs.swap(cur_row_objs);
    cur_row_objs.clear();
  }
}

static void
patch_group(std::vector<RunNode> &graph, size_t old_group, size_t new_group)
{
  for (size_t i = 0; i < graph.size(); ++i) {
    RunNode &node = graph[i];
    if (node.group == old_group)
      node.group = new_group;
  }
}

static size_t
find_or_assign_group(RunNode &node, std::vector<RunNode> &graph,
                     size_t new_group)
{
  if (node.flags & RUN_GROUPED)
    return node.group;

  if (node.adjacent.empty()) {
    node.flags |= (RUN_TOP | RUN_GROUPED);
    return node.group = new_group;
  }

  for (size_t i = node.adjacent.size(); 0 < i; --i) {
    RunNode &adj = graph[node.adjacent[i - 1]];
    size_t group = find_or_assign_group(adj, graph, new_group);
    if (i < node.adjacent.size() && new_group != group)
      patch_group(graph, new_group, group);
    new_group = group;
  }

  node.flags |= RUN_GROUPED;
  return node.group = new_group;
}

static void
define_objects(std::vector<RunNode> &graph)
{
  size_t next_group = 0;
  for (size_t i = graph.size(); 0 < i; --i) {
    RunNode &node = graph[i - 1];
    size_t group = find_or_assign_group(node, graph, next_group);
    if (group == next_group) {
      node.flags |= RUN_BOTTOM;
      ++next_group;
    }
  }
}

static void
fix_area_and_bound(Obj &obj, const Run &run)
{
  obj.area += run.end - run.start;
  int xend = obj.bound.x + obj.bound.width;
  obj.bound.x = std::min(obj.bound.x, run.start);
  obj.bound.width = std::max(xend, run.end) - obj.bound.x;
  int yend = obj.bound.y + obj.bound.height;
  obj.bound.y = std::min(obj.bound.y, run.row);
  obj.bound.height = std::max(yend, run.row + 1) - obj.bound.y;
}

static void
extract_objects(std::vector<RunNode> &graph, std::vector<Obj> &objs)
{
  const int maxint = std::numeric_limits<int>::max();
  assert(maxint + (-maxint) == 0);
  cv::Rect init_bound;
  init_bound.x = maxint;
  init_bound.y = maxint;
  init_bound.width = -maxint;
  init_bound.height = -maxint;

  typedef std::map<size_t, size_t> table_t;
  table_t table;
  Run run;
  Obj *obj;

  for (size_t i = 0; i < graph.size(); ++i) {
    RunNode &node = graph[i];
    run.row = node.row;
    run.start = node.start;
    run.end = node.end;
    run.flags = node.flags & (RUN_TOP | RUN_BOTTOM);

    table_t::iterator iter = table.find(node.group);
    if (iter != table.end())
      obj = &objs[iter->second];
    else {
      objs.push_back(Obj());
      obj = &objs.back();
      obj->color = node.color;
      obj->area = 0;
      obj->bound = init_bound;
      table[node.group] = objs.size() - 1;
    }

    obj->runs.push_back(run);
    fix_area_and_bound(*obj, run);
  }
}

void
objfind(const cv::Mat &img, std::vector<Obj> &objs)
{
  assert(img.depth() == CV_8U);

  std::vector<RunNode> graph;
  objfind_generate_run_graph(img, graph);

  define_objects(graph);

  assert(objs.size() == 0);
  extract_objects(graph, objs);
}

void
fillobj(cv::Mat &img, const Obj &obj, cv::Scalar color)
{
  for (size_t i = 0; i < obj.runs.size(); ++i) {
    const Run &run = obj.runs[i];
    cv::Point p1(run.start, run.row);
    cv::Point p2(run.end - 1, run.row);
    cv::line(img, p1, p2, color);
  }
}

void
fillgaps(const Obj &src, Obj &dst)
{
  dst.runs.clear();
  dst.area = 0;
  dst.color = src.color;
  dst.bound = src.bound;

  Run init;
  init.row = -1;
  init.start = std::numeric_limits<int>::max();
  init.end = 0;
  init.flags = RUN_TOP | RUN_BOTTOM;

  int toprow = src.runs[0].row;
  int botrow = src.runs.back().row;
  dst.runs.resize(botrow - toprow + 1, init);
  for (size_t i = 0; i < src.runs.size(); ++i) {
    const Run &sr = src.runs[i];
    assert(toprow <= sr.row && sr.row <= botrow);
    Run &dr = dst.runs[sr.row - toprow];
    if (dr.row != -1) {
      assert(dr.end - dr.start <= (int) dst.area);
      dst.area -= dr.end - dr.start;
    }
    dr.row = sr.row;
    dr.start = std::min(dr.start, sr.start);
    dr.end = std::max(dr.end, sr.end);
    dr.flags &= sr.flags;
    dst.area += dr.end - dr.start;
  }
}

static bool
area_comparator(const Obj &o1, const Obj &o2)
{
  return o2.area < o1.area;
}

void
sortobjs(std::vector<Obj> &objs)
{
  std::sort(objs.begin(), objs.end(), area_comparator);
}

//}  // namespace objfind
