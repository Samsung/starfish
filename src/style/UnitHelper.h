#ifndef __StarFishUnitHelper__
#define __StarFishUnitHelper__

#include "StarFish.h"

namespace StarFish {

// https://www.w3.org/TR/CSS2/syndata.html#value-def-length
const float unitPxPerCm = 96 / 2.54;
const float unitPxPerMm = unitPxPerCm / 10;
const float unitPxPerIn = 96;
const float unitPxPerPc = unitPxPerIn / 6;
const float unitPxPerPt = unitPxPerIn / 72;

float convertFromCmToPx(float value);
float convertFromMmToPx(float value);
float convertFromInToPx(float value);
float convertFromPcToPx(float value);
float convertFromPtToPx(float value);
float convertFromPxToPt(float value);

}

#endif
