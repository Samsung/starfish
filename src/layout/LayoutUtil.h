#ifndef __StarFishLayoutUtil__
#define __StarFishLayoutUtil__

#include <limits.h>
#include <limits>
#include <math.h>
#include <stdlib.h>

namespace StarFish {

inline int32_t saturatedAddition(int32_t a, int32_t b)
{
    uint32_t ua = a;
    uint32_t ub = b;
    uint32_t result = ua + ub;

    // Can only overflow if the signed bit of the two values match. If the signed
    // bit of the result and one of the values differ it did overflow.
    if (!((ua ^ ub) >> 31) & (result ^ ua) >> 31)
        result = std::numeric_limits<int>::max() + (ua >> 31);

    return result;
}

inline int32_t saturatedSubtraction(int32_t a, int32_t b)
{
    uint32_t ua = a;
    uint32_t ub = b;
    uint32_t result = ua - ub;

    // Can only overflow if the signed bit of the two values do not match. If the
    // signed bit of the result and the first value differ it did overflow.
    if ((ua ^ ub) >> 31 & (result ^ ua) >> 31)
        result = std::numeric_limits<int>::max() + (ua >> 31);

    return result;
}

// std::numeric_limits<T>::min() returns the smallest positive value for floating point types
template <typename T>
inline T defaultMinimumForClamp() { return std::numeric_limits<T>::min(); }
template <>
inline float defaultMinimumForClamp() { return -std::numeric_limits<float>::max(); }
template <>
inline double defaultMinimumForClamp() { return -std::numeric_limits<double>::max(); }
template <typename T>
inline T defaultMaximumForClamp() { return std::numeric_limits<T>::max(); }

template <typename T>
inline T clampTo(double value, T min = defaultMinimumForClamp<T>(), T max = defaultMaximumForClamp<T>())
{
    if (value >= static_cast<double>(max))
        return max;
    if (value <= static_cast<double>(min))
        return min;
    return static_cast<T>(value);
}
template <>
inline long long int clampTo(double, long long int, long long int); // clampTo does not support long long ints.

inline int clampToInteger(double value)
{
    return clampTo<int>(value);
}

static const int kFixedPointDenominator = 64;
const int intMaxForLayoutUnit = INT_MAX / kFixedPointDenominator;
const int intMinForLayoutUnit = INT_MIN / kFixedPointDenominator;

class LayoutUnit {
public:
    LayoutUnit()
        : m_value(0)
    {
    }
    LayoutUnit(int value) { setValue(value); }
    LayoutUnit(unsigned short value) { setValue(value); }
    LayoutUnit(unsigned value) { setValue(value); }
    LayoutUnit(unsigned long value)
    {
        m_value = clampTo<int>(value * kFixedPointDenominator);
    }

    LayoutUnit(unsigned long long value)
    {
        m_value = clampTo<int>(value * kFixedPointDenominator);
    }

    LayoutUnit(float value)
    {
        m_value = clampToInteger(value * kFixedPointDenominator);
    }

    LayoutUnit(double value)
    {
        m_value = clampToInteger(value * kFixedPointDenominator);
    }

    static LayoutUnit fromPixel(int value)
    {
        return LayoutUnit(value);
    }

    static LayoutUnit fromFloatCeil(float value)
    {
        LayoutUnit v;
        v.m_value = clampToInteger(ceilf(value * kFixedPointDenominator));
        return v;
    }

    static LayoutUnit fromFloatFloor(float value)
    {
        LayoutUnit v;
        v.m_value = clampToInteger(floorf(value * kFixedPointDenominator));
        return v;
    }

    static LayoutUnit fromFloatRound(float value)
    {
        if (value >= 0)
            return clamp(value + epsilon() / 2.0f);
        return clamp(value - epsilon() / 2.0f);
    }

    int toInt() const { return m_value / kFixedPointDenominator; }
    float toFloat() const { return static_cast<float>(m_value) / kFixedPointDenominator; }
    double toDouble() const { return static_cast<double>(m_value) / kFixedPointDenominator; }
    unsigned toUnsigned() const
    {
        STARFISH_ASSERT(m_value >= 0);
        return toInt();
    }

    operator int() const { return toInt(); }
    operator unsigned() const { return toUnsigned(); }
    operator float() const { return toFloat(); }
    operator double() const { return toDouble(); }
    operator bool() const { return m_value; }

    LayoutUnit& operator++()
    {
        m_value += kFixedPointDenominator;
        return *this;
    }

    inline int rawValue() const { return m_value; }
    inline void setRawValue(int value) { m_value = value; }
    void setRawValue(long long value)
    {
        STARFISH_ASSERT(value > std::numeric_limits<int>::min() && value < std::numeric_limits<int>::max());
        m_value = static_cast<int>(value);
    }

    LayoutUnit abs() const
    {
        LayoutUnit returnValue;
        returnValue.setRawValue(::abs(m_value));
        return returnValue;
    }
    int ceil() const
    {
        if (UNLIKELY(m_value >= INT_MAX - kFixedPointDenominator + 1))
            return intMaxForLayoutUnit;
        if (m_value >= 0)
            return (m_value + kFixedPointDenominator - 1) / kFixedPointDenominator;
        return toInt();
    }

    int round() const
    {
        if (m_value > 0)
            return saturatedAddition(rawValue(), kFixedPointDenominator / 2) / kFixedPointDenominator;
        return saturatedSubtraction(rawValue(), (kFixedPointDenominator / 2) - 1) / kFixedPointDenominator;
    }

    int floor() const
    {
        if (UNLIKELY(m_value <= INT_MIN + kFixedPointDenominator - 1))
            return intMinForLayoutUnit;
        if (m_value >= 0)
            return toInt();
        return (m_value - kFixedPointDenominator + 1) / kFixedPointDenominator;
    }

    float ceilToFloat() const
    {
        float floatValue = toFloat();
        if (static_cast<int>(floatValue * kFixedPointDenominator) == m_value)
            return floatValue;
        if (floatValue > 0)
            return nextafterf(floatValue, std::numeric_limits<float>::max());
        return nextafterf(floatValue, std::numeric_limits<float>::min());
    }

    LayoutUnit fraction() const
    {
        // Add the fraction to the size (as opposed to the full location) to avoid overflows.
        // Compute fraction using the mod operator to preserve the sign of the value as it may affect rounding.
        LayoutUnit fraction;
        fraction.setRawValue(rawValue() % kFixedPointDenominator);
        return fraction;
    }

    bool mightBeSaturated() const
    {
        return rawValue() == std::numeric_limits<int>::max()
            || rawValue() == std::numeric_limits<int>::min();
    }

    static float epsilon() { return 1.0f / kFixedPointDenominator; }

    static const LayoutUnit max()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::max();
        return m;
    }
    static const LayoutUnit min()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::min();
        return m;
    }

    // Versions of max/min that are slightly smaller/larger than max/min() to allow for roinding without overflowing.
    static const LayoutUnit nearlyMax()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::max() - kFixedPointDenominator / 2;
        return m;
    }
    static const LayoutUnit nearlyMin()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::min() + kFixedPointDenominator / 2;
        return m;
    }

    static LayoutUnit clamp(double value)
    {
        return clampTo<LayoutUnit>(value, LayoutUnit::min(), LayoutUnit::max());
    }

private:
    static bool isInBounds(int value)
    {
        return ::abs(value) <= std::numeric_limits<int>::max() / kFixedPointDenominator;
    }
    static bool isInBounds(unsigned value)
    {
        return value <= static_cast<unsigned>(std::numeric_limits<int>::max()) / kFixedPointDenominator;
    }
    static bool isInBounds(double value)
    {
        return ::fabs(value) <= std::numeric_limits<int>::max() / kFixedPointDenominator;
    }

    inline void setValue(int value)
    {
        if (value > intMaxForLayoutUnit)
            m_value = std::numeric_limits<int>::max();
        else if (value < intMinForLayoutUnit)
            m_value = std::numeric_limits<int>::min();
        else
            m_value = value * kFixedPointDenominator;
    }
    inline void setValue(unsigned value)
    {
        if (value >= static_cast<unsigned>(intMaxForLayoutUnit))
            m_value = std::numeric_limits<int>::max();
        else
            m_value = value * kFixedPointDenominator;
    }

    int m_value;
};

inline bool operator<=(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() <= b.rawValue();
}

inline bool operator<=(const LayoutUnit& a, float b)
{
    return a.toFloat() <= b;
}

inline bool operator<=(const LayoutUnit& a, int b)
{
    return a <= LayoutUnit(b);
}

inline bool operator<=(const float a, const LayoutUnit& b)
{
    return a <= b.toFloat();
}

inline bool operator<=(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) <= b;
}

inline bool operator>=(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() >= b.rawValue();
}

inline bool operator>=(const LayoutUnit& a, int b)
{
    return a >= LayoutUnit(b);
}

inline bool operator>=(const float a, const LayoutUnit& b)
{
    return a >= b.toFloat();
}

inline bool operator>=(const LayoutUnit& a, float b)
{
    return a.toFloat() >= b;
}

inline bool operator>=(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) >= b;
}

inline bool operator<(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() < b.rawValue();
}

inline bool operator<(const LayoutUnit& a, int b)
{
    return a < LayoutUnit(b);
}

inline bool operator<(const LayoutUnit& a, float b)
{
    return a.toFloat() < b;
}

inline bool operator<(const LayoutUnit& a, double b)
{
    return a.toDouble() < b;
}

inline bool operator<(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) < b;
}

inline bool operator<(const float a, const LayoutUnit& b)
{
    return a < b.toFloat();
}

inline bool operator>(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() > b.rawValue();
}

inline bool operator>(const LayoutUnit& a, double b)
{
    return a.toDouble() > b;
}

inline bool operator>(const LayoutUnit& a, float b)
{
    return a.toFloat() > b;
}

inline bool operator>(const LayoutUnit& a, int b)
{
    return a > LayoutUnit(b);
}

inline bool operator>(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) > b;
}

inline bool operator>(const float a, const LayoutUnit& b)
{
    return a > b.toFloat();
}

inline bool operator>(const double a, const LayoutUnit& b)
{
    return a > b.toDouble();
}

inline bool operator!=(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() != b.rawValue();
}

inline bool operator!=(const LayoutUnit& a, float b)
{
    return a != LayoutUnit(b);
}

inline bool operator!=(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) != b;
}

inline bool operator!=(const LayoutUnit& a, int b)
{
    return a != LayoutUnit(b);
}

inline bool operator==(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() == b.rawValue();
}

inline bool operator==(const LayoutUnit& a, int b)
{
    return a == LayoutUnit(b);
}

inline bool operator==(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) == b;
}

inline bool operator==(const LayoutUnit& a, float b)
{
    return a.toFloat() == b;
}

inline bool operator==(const float a, const LayoutUnit& b)
{
    return a == b.toFloat();
}

// For multiplication that's prone to overflow, this bounds it to LayoutUnit::max() and ::min()
inline LayoutUnit boundedMultiply(const LayoutUnit& a, const LayoutUnit& b)
{
    int64_t result = static_cast<int64_t>(a.rawValue()) * static_cast<int64_t>(b.rawValue()) / kFixedPointDenominator;
    int32_t high = static_cast<int32_t>(result >> 32);
    int32_t low = static_cast<int32_t>(result);
    uint32_t saturated = (static_cast<uint32_t>(a.rawValue() ^ b.rawValue()) >> 31) + std::numeric_limits<int>::max();
    // If the higher 32 bits does not match the lower 32 with sign extension the operation overflowed.
    if (high != low >> 31)
        result = saturated;

    LayoutUnit returnVal;
    returnVal.setRawValue(static_cast<int>(result));
    return returnVal;
}

inline LayoutUnit operator*(const LayoutUnit& a, const LayoutUnit& b)
{
    return boundedMultiply(a, b);
}

inline double operator*(const LayoutUnit& a, double b)
{
    return a.toDouble() * b;
}

inline float operator*(const LayoutUnit& a, float b)
{
    return a.toFloat() * b;
}

inline LayoutUnit operator*(const LayoutUnit& a, int b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned short b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned long b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned long long b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(unsigned short a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(unsigned a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(unsigned long a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(unsigned long long a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline float operator*(const float a, const LayoutUnit& b)
{
    return a * b.toFloat();
}

inline double operator*(const double a, const LayoutUnit& b)
{
    return a * b.toDouble();
}

inline LayoutUnit operator/(const LayoutUnit& a, const LayoutUnit& b)
{
    LayoutUnit returnVal;
    long long rawVal = static_cast<long long>(kFixedPointDenominator) * a.rawValue() / b.rawValue();
    returnVal.setRawValue(clampTo<int>(rawVal));
    return returnVal;
}

inline float operator/(const LayoutUnit& a, float b)
{
    return a.toFloat() / b;
}

inline double operator/(const LayoutUnit& a, double b)
{
    return a.toDouble() / b;
}

inline LayoutUnit operator/(const LayoutUnit& a, int b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned short b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned long b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned long long b)
{
    return a / LayoutUnit(b);
}

inline float operator/(const float a, const LayoutUnit& b)
{
    return a / b.toFloat();
}

inline double operator/(const double a, const LayoutUnit& b)
{
    return a / b.toDouble();
}

inline LayoutUnit operator/(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned short a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned long a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned long long a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator+(const LayoutUnit& a, const LayoutUnit& b)
{
    LayoutUnit returnVal;
    returnVal.setRawValue(saturatedAddition(a.rawValue(), b.rawValue()));
    return returnVal;
}

inline LayoutUnit operator+(const LayoutUnit& a, int b)
{
    return a + LayoutUnit(b);
}

inline float operator+(const LayoutUnit& a, float b)
{
    return a.toFloat() + b;
}

inline double operator+(const LayoutUnit& a, double b)
{
    return a.toDouble() + b;
}

inline LayoutUnit operator+(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) + b;
}

inline float operator+(const float a, const LayoutUnit& b)
{
    return a + b.toFloat();
}

inline double operator+(const double a, const LayoutUnit& b)
{
    return a + b.toDouble();
}

inline LayoutUnit operator-(const LayoutUnit& a, const LayoutUnit& b)
{
    LayoutUnit returnVal;
    returnVal.setRawValue(saturatedSubtraction(a.rawValue(), b.rawValue()));
    return returnVal;
}

inline LayoutUnit operator-(const LayoutUnit& a, int b)
{
    return a - LayoutUnit(b);
}

inline LayoutUnit operator-(const LayoutUnit& a, unsigned b)
{
    return a - LayoutUnit(b);
}

inline float operator-(const LayoutUnit& a, float b)
{
    return a.toFloat() - b;
}

inline LayoutUnit operator-(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) - b;
}

inline float operator-(const float a, const LayoutUnit& b)
{
    return a - b.toFloat();
}

inline LayoutUnit operator-(const LayoutUnit& a)
{
    LayoutUnit returnVal;
    returnVal.setRawValue(-a.rawValue());
    return returnVal;
}

// For returning the remainder after a division with integer results.
inline LayoutUnit intMod(const LayoutUnit& a, const LayoutUnit& b)
{
    // This calculates the modulo so that: a = static_cast<int>(a / b) * b + intMod(a, b).
    LayoutUnit returnVal;
    returnVal.setRawValue(a.rawValue() % b.rawValue());
    return returnVal;
}

inline LayoutUnit operator%(const LayoutUnit& a, const LayoutUnit& b)
{
    // This calculates the modulo so that: a = (a / b) * b + a % b.
    LayoutUnit returnVal;
    long long rawVal = (static_cast<long long>(kFixedPointDenominator) * a.rawValue()) % b.rawValue();
    returnVal.setRawValue(rawVal / kFixedPointDenominator);
    return returnVal;
}

inline LayoutUnit operator%(const LayoutUnit& a, int b)
{
    return a % LayoutUnit(b);
}

inline LayoutUnit operator%(int a, const LayoutUnit& b)
{
    return LayoutUnit(a) % b;
}

inline LayoutUnit& operator+=(LayoutUnit& a, const LayoutUnit& b)
{
    a.setRawValue(saturatedAddition(a.rawValue(), b.rawValue()));
    return a;
}

inline LayoutUnit& operator+=(LayoutUnit& a, int b)
{
    a = a + b;
    return a;
}

inline LayoutUnit& operator+=(LayoutUnit& a, float b)
{
    a = a + b;
    return a;
}

inline float& operator+=(float& a, const LayoutUnit& b)
{
    a = a + b;
    return a;
}

inline LayoutUnit& operator-=(LayoutUnit& a, int b)
{
    a = a - b;
    return a;
}

inline LayoutUnit& operator-=(LayoutUnit& a, const LayoutUnit& b)
{
    a.setRawValue(saturatedSubtraction(a.rawValue(), b.rawValue()));
    return a;
}

inline LayoutUnit& operator-=(LayoutUnit& a, float b)
{
    a = a - b;
    return a;
}

inline float& operator-=(float& a, const LayoutUnit& b)
{
    a = a - b;
    return a;
}

inline LayoutUnit& operator*=(LayoutUnit& a, const LayoutUnit& b)
{
    a = a * b;
    return a;
}
// operator*=(LayoutUnit& a, int b) is supported by the operator above plus LayoutUnit(int).

inline LayoutUnit& operator*=(LayoutUnit& a, float b)
{
    a = a * b;
    return a;
}

inline float& operator*=(float& a, const LayoutUnit& b)
{
    a = a * b;
    return a;
}

inline LayoutUnit& operator/=(LayoutUnit& a, const LayoutUnit& b)
{
    a = a / b;
    return a;
}
// operator/=(LayoutUnit& a, int b) is supported by the operator above plus LayoutUnit(int).

inline LayoutUnit& operator/=(LayoutUnit& a, float b)
{
    a = a / b;
    return a;
}

inline float& operator/=(float& a, const LayoutUnit& b)
{
    a = a / b;
    return a;
}

inline int snapSizeToPixel(LayoutUnit size, LayoutUnit location)
{
    LayoutUnit fraction = location.fraction();
    // NOTE: blink is using 'round', but phantomjs-webkit passes 'floor'.
    return (fraction + size).floor() - fraction.floor();
    //    return (fraction + size).round() - fraction.round();
}

inline int roundToInt(LayoutUnit value)
{
    return value.round();
}

inline int floorToInt(LayoutUnit value)
{
    return value.floor();
}

inline float roundToDevicePixel(LayoutUnit value, const float pixelSnappingFactor, bool needsDirectionalRounding = false)
{
    auto roundInternal = [&](float valueToRound)
    {
        return roundf((valueToRound * pixelSnappingFactor) / kFixedPointDenominator) / pixelSnappingFactor;
    };

    float adjustedValue = value.rawValue() - (needsDirectionalRounding ? LayoutUnit::epsilon() / 2.0f : 0);
    if (adjustedValue >= 0)
        return roundInternal(adjustedValue);

    // This adjusts directional rounding on negative halfway values. It produces the same direction for both negative and positive values.
    // It helps snapping relative negative coordinates to the same position as if they were positive absolute coordinates.
    float translateOrigin = fabsf(adjustedValue - LayoutUnit::fromPixel(1));
    return roundInternal(adjustedValue + (translateOrigin * kFixedPointDenominator)) - translateOrigin;
}

inline float floorToDevicePixel(LayoutUnit value, float pixelSnappingFactor)
{
    return floorf((value.rawValue() * pixelSnappingFactor) / kFixedPointDenominator) / pixelSnappingFactor;
}

inline float ceilToDevicePixel(LayoutUnit value, float pixelSnappingFactor)
{
    return ceilf((value.rawValue() * pixelSnappingFactor) / kFixedPointDenominator) / pixelSnappingFactor;
}

inline LayoutUnit absoluteValue(const LayoutUnit& value)
{
    return value.abs();
}

inline bool isIntegerValue(const LayoutUnit value)
{
    return value.toInt() == value;
}

class LayoutSize {
public:
    LayoutSize(LayoutUnit w, LayoutUnit h)
    {
        m_width = w;
        m_height = h;
    }

    void setWidth(LayoutUnit w)
    {
        m_width = w;
    }

    void setHeight(LayoutUnit h)
    {
        m_height = h;
    }

    LayoutUnit width() const
    {
        return m_width;
    }

    LayoutUnit height() const
    {
        return m_height;
    }

protected:
    LayoutUnit m_width, m_height;
};

class LayoutLocation {
public:
    LayoutLocation(LayoutUnit x, LayoutUnit y)
    {
        m_x = x;
        m_y = y;
    }

    void setX(LayoutUnit x)
    {
        m_x = x;
    }

    void setY(LayoutUnit y)
    {
        m_y = y;
    }

    LayoutUnit x() const
    {
        return m_x;
    }

    LayoutUnit y() const
    {
        return m_y;
    }

protected:
    LayoutUnit m_x, m_y;
};

class LayoutRect {
public:
    LayoutRect(LayoutUnit x, LayoutUnit y, LayoutUnit w, LayoutUnit h)
        : m_location(x, y)
        , m_size(w, h)
    {
    }

    LayoutUnit x() const { return m_location.x(); }
    LayoutUnit y() const { return m_location.y(); }
    LayoutUnit width() const { return m_size.width(); }
    LayoutUnit height() const { return m_size.height(); }

    void setX(LayoutUnit x) { m_location.setX(x); }
    void setY(LayoutUnit y) { m_location.setY(y); }
    void setWidth(LayoutUnit width) { m_size.setWidth(width); }
    void setHeight(LayoutUnit height) { m_size.setHeight(height); }

    bool contains(LayoutUnit px, LayoutUnit py) const
    {
        return px >= x() && px < (x() + width()) && py >= y() && py < (y() + height());
    }

private:
    LayoutLocation m_location;
    LayoutSize m_size;
};

class LayoutBoxSurroundData {
public:
    LayoutBoxSurroundData(LayoutUnit top = 0, LayoutUnit right = 0, LayoutUnit bottom = 0, LayoutUnit left = 0)
    {
        m_top = top;
        m_right = right;
        m_bottom = bottom;
        m_left = left;
    }

    LayoutUnit top()
    {
        return m_top;
    }

    LayoutUnit right()
    {
        return m_right;
    }

    LayoutUnit bottom()
    {
        return m_bottom;
    }

    LayoutUnit left()
    {
        return m_left;
    }

    void setTop(LayoutUnit v)
    {
        m_top = v;
    }

    void setRight(LayoutUnit v)
    {
        m_right = v;
    }

    void setBottom(LayoutUnit v)
    {
        m_bottom = v;
    }

    void setLeft(LayoutUnit v)
    {
        m_left = v;
    }

protected:
    LayoutUnit m_top;
    LayoutUnit m_right;
    LayoutUnit m_bottom;
    LayoutUnit m_left;
};
}

#endif
