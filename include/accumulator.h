#ifndef ACC_H
#define ACC_H

#include <math.h>

class Accumulator
{
public:
  Accumulator(int len = 60);
  ~Accumulator();
  bool add(float val);
  bool full();
  void reset();
  float stddev();
  float avg();
  float median();
  float min();
  float max();
  int count();

private:
  float *_vals;
  int _count = 0;
  int _len;
  bool _sorted = false;
  void sort();
};

class Trend
{
public:
  Trend(int len = 30);
  ~Trend();
  float calc(float value);

private:
  float *_vals;
  int _i = 0;
  size_t _len;
};
#endif