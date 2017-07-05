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

#ifndef __StarFishMediaQueryResult__
#define __StarFishMediaQueryResult__

namespace StarFish {

class MediaQueryResult : public gc {
public:
    MediaQueryResult(MediaQueryExp* expression, bool result)
        : m_expression(expression)
        , m_result(result)
    {
    }

    MediaQueryExp* expression() const
    {
        return m_expression;
    }

    bool result() const
    {
        return m_result;
    }

private:
    MediaQueryExp* m_expression;
    bool m_result;
};

} /* namespace StarFish */

#endif /* __StarFishMediaQueryResult__ */
