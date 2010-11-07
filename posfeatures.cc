#include "features.h"

static double
score_position(objs_t &objs, size_t subject, bool top)
{
  const Obj &obj = objs[subject];
  double tally = 0.;

  for (size_t i = 0; i < objs.size(); ++i) {
    if (i == subject) continue;

    double wd = obj.bound.width - objs[i].bound.width;
    double hd = obj.bound.height - objs[i].bound.height;
    double scale = obj.bound.width * obj.bound.height;
    double size_diff = std::fabs(wd * hd / scale);
    double pos_diff;
    if (top)
      pos_diff = std::fabs(obj.bound.y - objs[i].bound.y);
    else
      pos_diff = std::fabs(obj.bound.y + obj.bound.height -
                           objs[i].bound.y - objs[i].bound.height);
    pos_diff /= obj.bound.height;

    tally += std::exp(-size_diff * pos_diff);
  }

  return tally / objs.size();
}

double
TopPositionFeature::describe(objs_t &objs, size_t subject)
{
  return score_position(objs, subject, true);
}

double
BottomPositionFeature::describe(objs_t &objs, size_t subject)
{
  return score_position(objs, subject, false);
}
