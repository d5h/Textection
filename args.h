#ifndef ARGS_INCLUDED
#define ARGS_INCLUDED 1

#include <string>
#include <vector>

#define ARG_DEFAULT_1 8.0

void
parse_args(char **argv,
           std::vector<double> &params,
           std::vector<std::string> &args);

#endif //ARGS_INCLUDED
