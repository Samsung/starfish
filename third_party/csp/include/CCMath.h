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
 * \brief CCMath class header file
 * \author Youngil Choi <duddlf.choi@samsung.com>
 * \date 2006-12-01
 */




//! CCMath class
class CCMath
{
private:
	//! The private constructor that prevents instancing
	CCMath(void) {}

public:
	//! Generates a pseudorandom number.
	static long Random(unsigned long seed = 0);
	//! Calculates the cosine of a radian.
	static double Cos(double radian);
	//! Calculates the sine of a radian.
	static double Sin(double radian);
	//! Calculates the tangent of a radian.
	static double Tan(double radian);
	//! Calculates the arcsine of a radian.
	static double Asin(double radian);
	//! Calculates the arccosine of a radian.
	static double Acos(double radian);
	//! Calculates the arctangent of a radian.
	static double Atan(double radian);
	//! Calculates the arctangent of two variables.
	static double Atan2(double y, double x);
	//! Calculatess the floating-point remainder.
	static double Fmod(double x, double y);
	//! Calculates the logarithm.
	static double Log(double x);
	//! Calculates the base 10 logarithm.
	static double Log10(double x);
	//! Calculates x raised to the power of y.
	static double Pow(double x, double y);
	//! Calculates the exponential.
	static double Exp(double x);
	//! Calculates the square root.
	static double Sqrt(double x);
	//! Calcuates the largest integral value not greater than argument
	static double Floor(double x);
	//! Calcuates the smallest integral value not less than argument
	static double Ceil(double x);
};




/*!
 * \class CCMath
 *
 * CCMath class provides the mathematics functions.
 */




/*!
 * \fn long CCMath::Random(unsigned long seed)
 *
 * This function generates a pseudo-random number.
 *
 * \param[in] seed (unsigned long)
 *        Seed for random-number generation.
 *
 * \return (long)
 *        A pseudo-random number.
 */




/*!
 * \fn double CCMath::Sin(double x)
 *
 * This function calculates the sine of a radian.
 *
 * \param[in] x (double)
 *        Angle in radians
 *
 * \return (double)
 *        Sine value
 *
 * \see CCMath::Cos()
 */




/*!
 * \fn double CCMath::Cos(double x)
 *
 * This function calculates the cosine of a radian.
 *
 * \param[in] x (double)
 *        Angle in radians
 *
 * \return (double)
 *        Cosine value
 *
 * \see CCMath::Sin()
 */




/*!
 * \fn double CCMath::Tan(double x)
 *
 * This function calculates the tangent of a radian.
 *
 * \param[in] x (double)
 *        Angle in radians
 *
 * \return (double)
 *        Tangent value
 *
 * \see CCMath::Atan(), CCMath::Atan2()
 */




/*!
 * \fn double CCMath::Asin(double x)
 *
 * This function calculates the arcsine of a radian.
 *
 * \param[in] x (double)
 *        Sine value
 *
 * \return (double)
 *        arcsine value in radians
 *
 * \see CCMath::Sin()
 */




/*!
 * \fn double CCMath::Acos(double x)
 *
 * This function calculates the arctangent of a radian.
 *
 * \param[in] x (double) 
 *        Tangent value
 *
 * \return (double)
 *        Arctangent value in radians
 * 
 * \see CCMath::Cos()
 */




/*!
 * \fn double CCMath::Atan(double x)
 *
 * This function calculates the arctangent of a radian.
 *
 * \param[in] x (double)
 *        Tangent value
 *
 * \return (double)
 *        Arctangent value in radians
 *
 * \see CCMath::Tan(), CCMath::Atan2()
 */




/*!
 * \fn double CCMath::Atan2(double y, double x)
 *
 * This function calculates the arctangent of two variables x and y.
 * It is similar to calculating the arc tangent of y / x, except
 * that the signs of both arguments are used to determine the quadrant of
 * the result.
 *
 * \param[in] y (double) 
 *        the first variable of arctangent value
 * \param[in] x (double)
 *        the second variable of arctangent value
 *
 * \return (double) 
 *        Arctangent value in radians
 *
 * \see CCMath::Tan(), CCMath::Atan()
 */




/*!
 * \fn double CCMath::Fmod(double x, double y)
 *
 * This function computes the remainder of dividing x by y.
 * The return value is x - n * y, where n is the quotient of x / y,
 * rounded towards zero to an integer.
 *
 * \param[in] x (double)
 *        A floating-point value
 * \param[in] y (double)
 *        A floating-point value
 *
 * \return (double)
 *        The Fmod() function returns the remainder, unless \p y is zero, when the function fails.
 */




/*!
 * \fn double CCMath::Log(double x)
 *
 * The Log() function returns the natural logarithm of x.
 *
 * \param[in] x (double)
 *        A value whose logarithm is to be found
 *
 * \return (double)
 *        The natural logarithm of x.
 *
 * \see CCMath::Log10()
 */




/*!
 * \fn double CCMath::Log10(double x)
 *
 * The Log10() function returns the base 10 logarithm of x.
 *
 * \param[in] x (double)
 *        A value whose logarithm is to be found
 *
 * \return (double)
 *        The base 10 logarithm of x.
 *
 * \see CCMath::Log10()
 */




/*!
 * \fn double CCMath::Pow(double x, double y)
 *
 * The Pow() function returns the value of x raised to the power of y.
 *
 * \param[in] x (double)
 *        The base
 * \param[in] y (double)
 *        The exponent
 *
 * \return (double)
 *        The value of x raised to the power of y
 */




/*!
 * \fn double CCMath::Exp(double x)
 *
 * The exp() function returns the value of e (the base of natural
 * logarithms) raised to the power of x.
 *
 * \param[in] x (double)
 *        A floating-point value
 *
 * \return (double)
 *        The exponential value of x.
 */




/*!
 * \fn double CCMath::Sqrt(double x)
 *
 * The Sqrt() function returns the non-negative square root of x.
 *
 * \param[in] x (double)
 *        A non-negative floating-point value
 *
 * \return (double)
 *        The non-negative square root of x.
 */




/*!
 * \fn double CCMath::Floor(double x)
 *
 * The Floor() function returns the non-negative square root of x.
 *
 * \param[in] x (double)
 *        A floating-point value
 *
 * \return (double)
 *        The largest integral value not greater than argument.
 */




/*!
 * \fn double CCMath::Ceil(double x)
 *
 * The CeiL() function returns the smallest integral value not less than argument
 *
 * \param[in] x (double)
 *        A floating-point value
 *
 * \return (double)
 *        The smallest integral value not less than argument.
 */




