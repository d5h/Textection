#ifndef IMAGE_INCLUDED
#define IMAGE_INCLUDED 1

#include <vector>

#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv/cv.h>

#include "objfind.h"

extern void
get_sorted_objects_from_image(const cv::Mat &image,
                              std::vector<Obj> &objects,
                              const std::vector<double> &params);

#endif  // IMAGE_INCLUDED
