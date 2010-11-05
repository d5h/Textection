#ifndef KEYCODE_INCLUDED
#define KEYCODE_INCLUDED 1

#include <cctype>

static const int ESC_CODE = 27;
// These come from GDK
static const int LSHIFT_CODE = 0xffe1;
static const int RSHIFT_CODE = 0xffe2;

static const int TAB_CODE = 9;
static const int SPACE_CODE = 32;

static inline int
key_code(int code)
{
  return code & 0xffff;
}

static inline int
key_control(int code)
{
  return code >> 16;
}

static inline int
key_upper_control(int code)
{
  // The state flags come from GDK.
  // 1 = either shift
  // 2 = caps lock
  int flags = key_control(code) & 0x3;
  return (flags & 1) ^ (flags >> 1);
}

static inline int
key_ascii(int code)
{
  int val = key_code(code);
  if (val < 128 && key_upper_control(code))
    val = std::toupper(val);
  return val;
}

#endif //KEYCODE_INCLUDED
