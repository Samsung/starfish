#include "StarFishConfig.h"
#include "UnitHelper.h"

namespace StarFish {

extern int g_screenDpi;

float convertFromCmToPx(float value)
{
    return value * unitPxPerCm;
}

float convertFromMmToPx(float value)
{
    return value * unitPxPerMm;
}

float convertFromInToPx(float value)
{
    return value * unitPxPerIn;
}

float convertFromPcToPx(float value)
{
    return value * unitPxPerPc;
}

float convertFromPtToPx(float value)
{
    return value * unitPxPerPt;
}

float convertFromPxToPt(float value)
{
    return value / unitPxPerPt;
}
}
