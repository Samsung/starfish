/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/modules/location/Coordinates.h"
#include "core/dom/Document.h"

namespace Starfish {

Coordinates::Coordinates(Document* document, double latitude, double longitude,
                         Nullable<double> altitude, double accuracy,
                         Nullable<double> altitudeAccuracy,
                         Nullable<double> heading, Nullable<double> speed)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_latitude(latitude)
    , m_longitude(longitude)
    , m_altitude(altitude)
    , m_accuracy(accuracy)
    , m_altitudeAccuracy(altitudeAccuracy)
    , m_heading(heading)
    , m_speed(speed)
{
}

ScriptBindingInstance* Coordinates::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}
}
