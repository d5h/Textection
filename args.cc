#include <iostream>

#include <boost/lexical_cast.hpp>

#include "args.h"

void
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
