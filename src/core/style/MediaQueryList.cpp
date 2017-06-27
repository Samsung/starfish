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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryList.h"
#include "core/style/MediaQuerySet.h"

namespace StarFish {

ScriptBindingInstance* MediaQueryList::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

String* MediaQueryList::media() const
{
    return m_media->mediaText();
}

bool MediaQueryList::matches()
{
    return m_evaluator->eval(m_media);
}

} /* namespace StarFish */
