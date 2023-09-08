#pragma once

#define DISALLOW_COPY_AND_ASSIGN(class_name) \
  class_name(const class_name&) = delete;    \
  void operator=(const class_name&) = delete;

#define DISALLOW_COPY_AND_ASSIGN(class_name) \
 private:                                    \
  class_name(const class_name&);             \
  void operator=(const class_name&);
