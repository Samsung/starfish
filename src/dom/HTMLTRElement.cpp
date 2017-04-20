/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "dom/Document.h"
#include "dom/HTMLTRElement.h"
#include "platform/window/Window.h"

namespace StarFish {
String* HTMLTRElement::localName()
{
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_trTagName.localName();
}

QualifiedName HTMLTRElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_trTagName;
}
}
