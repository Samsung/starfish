/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*!
 * \file
 * \brief CCMath class source file
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#include <math.h>
#include <stdlib.h>

#include <CSP.h>




long CCMath::Random(unsigned long seed)
{
	if (seed != 0)
	{
		srand(seed);
	}

#if defined (_WIN32)

	return ((rand() & 0x7f) << 24) | ((rand() & 0xff) << 16) | ((rand() & 0xff) << 8) | (rand() & 0xff);

#elif defined (_LINUX)

	return rand();

#endif
}




double CCMath::Sin(double x)	
{
	return sin(x);
}




double CCMath::Cos(double x)
{
	return cos(x);
}




double CCMath::Tan(double x)
{
	return tan(x);
}




double CCMath::Asin(double x)
{
	return asin(x);
}




double CCMath::Acos(double x)
{
	return acos(x);
}




double CCMath::Atan(double x)
{
	return atan(x);
}




double CCMath::Atan2(double y, double x)
{
	return atan2(y, x);
}




double CCMath::Fmod(double x, double y)
{
	return fmod(x, y);
}




double CCMath::Log(double x)
{
	return log(x);
}




double CCMath::Log10(double x)
{
	return log10(x);
}




double CCMath::Pow(double x, double y)
{
	return pow(x, y);
}




double CCMath::Exp(double x)	
{
	return exp(x); 
}




double CCMath::Sqrt(double x)
{
	return sqrt(x); 
}




double CCMath::Floor(double x)
{
	return floor(x); 
}




double CCMath::Ceil(double x)
{
	return ceil(x); 
}




