#include <iostream>
#include <string>
#include <vector>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "args.h"
#include "classifier.h"
#include "image.h"

struct CharInfo {
  int c;
  cv::Rect bound;

  CharInfo(int c, cv::Rect bound)
    : c(c), bound(bound)
  {}
};

static void
process_image(const std::string &filename, const std::vector<double> &params)
{
  cv::Mat img = cv::imread(filename);
  std::vector<Obj> objs;
  get_sorted_objects_from_image(img, objs, params);

  std::vector<CharInfo> characters;
  for (size_t i = 0; i < objs.size(); ++i) {
    int c = classify(objs, i);
    if (c)
      characters.push_back(CharInfo(c, objs[i].bound));
  }

  std::cout << characters.size() << " characters found.\n";
  std::cout << objs.size() - characters.size() << " objects discarded.\n";
  for (size_t i = 0; i < characters.size(); ++i)
    std::cout << static_cast<char>(characters[i].c);
  std::cout << std::endl;
}

int
main(int argc, char **argv)
{
  std::vector<std::string> args;
  std::vector<double> params;
  params.push_back(ARG_DEFAULT_1);
  parse_args(argv + 1, params, args);

  for (size_t i = 0; i < args.size(); ++i)
    process_image(args[i], params);

  return 0;
}
