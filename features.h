#ifndef FEATURES_INCLUDED
#define FEATURES_INCLUDED 1

#include <cmath>
#include <string>
#include <vector>

#include "objfind.h"

typedef const std::vector<Obj> objs_t;

class Feature
{
public:
  virtual const char *name() = 0;
  virtual double describe(objs_t &, size_t) = 0;
  virtual ~Feature() {}
};

class AspectRatioFeature : public Feature
{
public:
  virtual const char *name() { return "AspectRatio"; }
  virtual double describe(objs_t &objs, size_t subject)
  {
    const Obj &obj = objs[subject];
    return static_cast<double>(obj.bound.height) / obj.bound.width;
  }
};

class TopPositionFeature: public Feature
{
public:
  virtual const char *name() { return "TopPosition"; }
  virtual double describe(objs_t &objs, size_t subject);
};

class BottomPositionFeature: public Feature
{
public:
  virtual const char *name() { return "BottomPosition"; }
  virtual double describe(objs_t &objs, size_t subject);
};

#endif // FEATURES_INCLUDED
