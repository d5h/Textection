#include <cassert>
#include <cctype>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "objfind.h"

std::string window_name = "Textection training";
std::vector<double> params;

static void
show(const cv::Mat &img)
{
  cv::imshow(window_name, img);
  int key = cv::waitKey();
  std::cout << key << std::endl;
  if (key == 'q')
    std::exit(0);
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

static void
process_img(const std::string &name)
{
  cv::Mat img = cv::imread(name);
  cv::Mat final, gray, pyrd, pyru;
  cv::pyrDown(img, pyrd);
  cv::pyrUp(pyrd, pyru);
  cv::cvtColor(pyru, gray, CV_BGR2GRAY);
  cv::equalizeHist(gray, final);
  truncColors(final, params[0]);
  show(final);

  std::vector<Obj> objs;
  objfind(final, objs);
  sortobjs(objs);

  for (size_t i = 0; i < objs.size(); ++i) {
    cv::Mat m = img.clone();
    fillobj(m, objs[i], cv::Scalar(255, 255, 0));
    show(m);
  }
}

int
main(int argc, char **argv)
{
  cv::namedWindow(window_name, CV_WINDOW_AUTOSIZE);

  std::vector<std::string> args;
  params.push_back(16);
  parse_args(argv + 1, params, args);

  for (size_t n = 0; n < args.size(); ++n) {
    std::cout << args[n] << std::endl;
    process_img(args[n]);
  }

  return 0;
}
