#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

// returns a random integer between x and y
inline int RandInt(int x, int y)
{
	return rand() % (y - x + 1) + x;
}

// returns a random float between zero and 1
inline double RandFloat()
{
	return (rand()) / (RAND_MAX + 1.0);
}

// returns a random bool
inline bool RandBool()
{
	if(RandInt(0,1)) return true;
	else return false;
}

// returns a random float in the range -1 < n < 1
inline double RandClamped()
{
	return RandFloat() - RandFloat();
}

// converts an integer to a string
inline string itos(int arg)
{
    ostringstream buffer;
    buffer << arg;	

    return buffer.str();		
}

// converts a float to a string
inline string ftos(float arg)
{
    ostringstream buffer;
    buffer << arg;	
	
    return buffer.str();		
}

// clamps the first argument between the second two
inline void Clamp(
	double &arg,
	const double min,
	const double max
) {
	if(arg < min) arg = min;
	if(arg > max) arg = max;
}


// rounds a double up or down depending on its value
inline int Rounded(double val)
{
  int integral = (int)val;
  double mantissa = val - integral;

  return (mantissa < 0.5 ? integral : ++integral);
}

// rounds a double up or down depending on whether its 
// mantissa is higher or lower than offset
inline int RoundUnderOffset(double val, double offset)
{
  int integral = (int)val;
  double mantissa = val - integral;
  
  return (mantissa < offset ? integral : ++integral);
}

#endif