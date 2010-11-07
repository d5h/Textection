#ifndef OBJFIND_INCLUDED
#define OBJFIND_INCLUDED 1

#include <vector>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>

static const unsigned int RUN_TOP = 1;
static const unsigned int RUN_BOTTOM = 2;
static const unsigned int RUN_GROUPED = 4;

struct RunBase {
  int row;
  int start;
  int end;
};

struct Run : public RunBase {
  unsigned int flags;
};

struct Obj {
  std::vector<Run> runs;
  size_t area;
  int color;  // Could be a Scalar if we want multiple colors
  cv::Rect bound;
};

void objfind(const cv::Mat &img, std::vector<Obj> &objs);
void fillobj(cv::Mat &img, const Obj &obj, cv::Scalar color);
void fillgaps(const Obj &src, Obj &dst);
void sortobjs(std::vector<Obj> &objs);

#endif  // OBJFIND_INCLUDED
