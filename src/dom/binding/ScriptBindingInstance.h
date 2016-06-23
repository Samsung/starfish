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
