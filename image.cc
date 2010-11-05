#include "image.h"

static void
trunc_colors(cv::Mat &m, int ncolors)
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

void
get_sorted_objects_from_image(const cv::Mat &img,
                              std::vector<Obj> &objs,
                              const std::vector<double> &params)
{
  cv::Mat final, gray, pyrd, pyru;
  cv::pyrDown(img, pyrd);
  cv::pyrUp(pyrd, pyru);
  cv::cvtColor(pyru, gray, CV_BGR2GRAY);
  cv::equalizeHist(gray, final);
  trunc_colors(final, params[0]);

  objfind(final, objs);
  sortobjs(objs);
}
