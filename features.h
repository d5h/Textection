#ifndef FEATURES_INCLUDED
#define FEATURES_INCLUDED 1

#include <string>

#include "objfind.h"

class Feature
{
public:
  virtual const char *name() = 0;
  virtual double describe(const Obj &) = 0;
  virtual ~Feature() {}
};

class AspectRatioFeature : public Feature
{
public:
  virtual const char *name() { return "AspectRatio"; }
  virtual double describe(const Obj &obj)
  {
    return static_cast<double>(obj.bound.height) / obj.bound.width;
  }
};

#endif // FEATURES_INCLUDED
