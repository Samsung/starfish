#ifndef __StarFishCharacterData__
#define __StarFishCharacterData__

#include "dom/Node.h"

namespace StarFish {

class Text;

class CharacterData : public Node {
public:
    CharacterData(Document* document, String* data)
        : Node(document)
    {
        m_data = data;
    }

    /* 4.4 Interface Node */

    virtual NodeType nodeType() = 0;

    virtual void setNodeValue(String* val)
    {
        String* d = val;
        if (d == nullptr) {
            d = String::emptyString;
        }
        setData(d);
    }

    virtual void setTextContent(String* val)
    {
        setNodeValue(val);
    }

    virtual String* nodeValue()
    {
        return data();
    }

    /* 4.9. Interface CharacterData */

    String* data()
    {
        return m_data;
    }

    void setData(String* data)
    {
        STARFISH_ASSERT(data);
        String* oldData = m_data;
        m_data = data;

        setNeedsLayout();

        notifyDOMEventToParentTree(parentNode(), [oldData, data](Node* parent) {
            parent->didCharacterDataModified(oldData, data);
        });
    }

    size_t length()
    {
        return m_data->length();
    }

    /* Other methods (not in DOM API) */

    virtual bool isCharacterData() const
    {
        return true;
    }

    Text* asText()
    {
        STARFISH_ASSERT(isText());
        return (Text*)this;
    }

    virtual Node* clone() = 0;

    virtual void paint(Canvas* canvas)
    {
    }

    virtual Node* hitTest(float x, float y)
    {
        return NULL;
    }

    static std::string replaceAll(const std::string& str, const std::string& pattern, const std::string& replace)
    {
        std::string result = str;
        std::string::size_type pos = 0;
        std::string::size_type offset = 0;

        while ((pos = result.find(pattern, offset)) != std::string::npos) {
            result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
            offset = pos + replace.size();
        }

        return result;
    }

    virtual void dump()
    {
        Node::dump();

        std::string str = m_data->utf8Data();
        str = replaceAll(str, "\n", "\\n");
        printf("data:%s ", str.data());
    }

private:
    String* m_data;
};
}

#endif
