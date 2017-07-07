MESSAGE(STATUS "Using bundled Findlibyaml-cpp.cmake...")
  FIND_PATH(
  LIBYAMLCPP_INCLUDE_DIR
  yaml.h
  /usr/local/include/yaml-cpp
  /usr/include/
  )

FIND_LIBRARY(
  LIBYAMLCPP_LIBRARIES NAMES libyaml-cpp.a
  PATHS /usr/lib/ /usr/local/lib/ /usr/lib64 /usr/local/lib64
  )
