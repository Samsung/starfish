/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishScriptBindingInstance__
#define __StarFishScriptBindingInstance__


namespace StarFish {

class ScriptBindingInstance : public gc {
public:
    ScriptBindingInstance();
    ~ScriptBindingInstance();
    void enter();
    void exit();
    void close();
    void initBinding(StarFish* window);
    void* data()
    {
        return m_data;
    }
    void evaluate(String* str);
protected:
    void* m_data;
    size_t m_enterCount;
};

class ScriptBindingInstanceEnterer {
public:
    ScriptBindingInstanceEnterer(ScriptBindingInstance& instance)
        : m_instance(instance)
    {
        m_instance.enter();
    }

    ScriptBindingInstanceEnterer(ScriptBindingInstance* instance)
        : m_instance(*instance)
    {
        m_instance.enter();
    }

    ~ScriptBindingInstanceEnterer()
    {
        m_instance.exit();
    }
protected:
    ScriptBindingInstance& m_instance;
};

}

#endif
