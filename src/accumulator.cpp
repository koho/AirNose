#include "accumulator.h"

Accumulator::Accumulator(int len)
{
  _len = len;
  _vals = new float[len];
  reset();
}

Accumulator::~Accumulator()
{
  delete[] _vals;
  _vals = nullptr;
}

bool Accumulator::add(float val)
{
  if (_count == _len)
  {
    return false;
  }
  _vals[_count++] = val;
  _sorted = false;
  return true;
}

bool Accumulator::full()
{
  return _count == _len;
}

int Accumulator::count()
{
  return _count;
}

void Accumulator::reset()
{
  if (_count == 0)
    return;
  _count = 0;
  for (int i = 0; i < _len; i++)
  {
    _vals[i] = 0;
  }
  _sorted = false;
}

float Accumulator::stddev()
{
  if (_count == 0)
    return 0;

  float u = avg();
  if (u < 0)
    return -1;
  float t = 0;
  for (int i = 0; i < _count; i++)
  {
    t += (_vals[i] - u) * (_vals[i] - u);
  }
  return sqrt(t / _count);
}

float Accumulator::median()
{
  if (_count == 0)
    return 0;
  sort();
  return _vals[_count / 2];
}

void Accumulator::sort()
{
  if (_sorted)
    return;
  _sorted = true;
  for (int i = 0; i < _count; i++)
  {
    for (int j = 0; j < i; j++)
    {
      if (_vals[i] < _vals[j])
      {
        float t = _vals[j];
        _vals[j] = _vals[i];
        _vals[i] = t;
      }
    }
  }
}

float Accumulator::avg()
{
  if (_count == 0)
    return 0;

  float t = 0;
  for (int i = 0; i < _count; i++)
  {
    t += _vals[i];
  }
  return t / _count;
}

float Accumulator::min()
{
  if (_count == 0)
    return 0;

  float t = _vals[0];
  for (int i = 0; i < _count; i++)
  {
    if (t > _vals[i])
    {
      t = _vals[i];
    }
  }
  return t;
}

float Accumulator::max()
{
  if (_count == 0)
    return 0;

  float t = _vals[0];
  for (int i = 0; i < _count; i++)
  {
    if (_vals[i] > t)
    {
      t = _vals[i];
    }
  }
  return t;
}

Trend::Trend(int len) {
  _len = len * 2;
  _vals = new float[_len];
  for (size_t i = 0; i < _len; i++)
  {
    _vals[i] = NAN;
  }
}

Trend::~Trend()
{
  delete[] _vals;
  _vals = nullptr;
}

float Trend::calc(float value)
{
  _vals[_i] = value;
  _i = (_i + 1) % _len;
  float last = 0, current = 0;
  int nLast = 0, nCurrent = 0;
  for (size_t i = _i, count = 0; count < _len; count++, i = (i + 1) % _len)
  {
    if (!isnan(_vals[i]))
    {
      if (count < _len / 2)
      {
        last += _vals[i];
        nLast++;
      }
      else
      {
        if (_vals[i] > current)
          current = _vals[i];
        nCurrent++;
      }
    }
  }
  if (nLast == 0 || nCurrent == 0)
  {
    return NAN;
  }
  return current - (last / nLast);
}
