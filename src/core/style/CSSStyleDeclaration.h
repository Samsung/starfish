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

#ifndef __StarFishCSSStyleDeclaration__
#define __StarFishCSSStyleDeclaration__

#include "binding/ScriptWrappable.h"
#include "core/style/Style.h"

namespace StarFish {

class CSSStyleDeclaration : public ScriptWrappable {
    friend class StyleResolver;

public:
    enum StyleType {
        ExternalStyle,
        InternalStyle,
        InlineStyle,
    };

    CSSStyleDeclaration(Element* element = nullptr,
                        StyleType styleType = InternalStyle)
        : ScriptWrappable(this)
    {
        m_element = element;
        m_styleType = styleType;
    }

    void addValuePair(CSSStyleValuePair p);

    void clear();

    CSSStyleDeclaration* clone(Element* element);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCSSStyleDeclaration() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    StyleType styleType()
    {
        return m_styleType;
    }

    void setStyleType(StyleType styleType)
    {
        m_styleType = styleType;
    }

    String* generateCSSText() const;

    void notifyNeedsStyleRecalc();

    void tokenizeCSSValue(GCVector<String*>* tokens, String* src,
                          String* seperator = String::emptyString,
                          bool isCaseSensitive = false);

    String* Border();
    String* BorderTop();
    String* BorderRight();
    String* BorderBottom();
    String* BorderLeft();
    String* Background();
    String* BackgroundRepeat();
    String* BackgroundPosition();
    String* Font();
    String* Transition();
#define DECLARE_ATTRIBUTE_GETTER(name, ...) String* name();
    FOR_EACH_STYLE_ATTRIBUTE(DECLARE_ATTRIBUTE_GETTER)
#undef DECLARE_ATTRIBUTE_GETTER

    void addCSSValuePair(CSSStyleValuePair::KeyKind name,
                         CSSStyleValuePair ret);

    void removeCSSValuePair(CSSStyleValuePair::KeyKind name);

    void setBorder(String* value, bool isImportant);
    void setBorderTop(String* value, bool isImportant);
    void setBorderRight(String* value, bool isImportant);
    void setBorderBottom(String* value, bool isImportant);
    void setBorderLeft(String* value, bool isImportant);
    void setBackground(String* value, bool isImportant);
    void setBackgroundRepeat(String* value, bool isImportant);
    void setBackgroundPosition(String* value, bool isImportant);
    void setMargin(String* value, bool isImportant);
    void setPadding(String* value, bool isImportant);
    void setBorderWidth(String* value, bool isImportant);
    void setBorderStyle(String* value, bool isImportant);
    void setBorderColor(String* value, bool isImportant);
    void setFont(String* value, bool isImportant);
    void setTransition(String* value, bool isImportant);

#define DECLARE_ATTRIBUTE_SETTER(name, ...) \
    void set##name(String* value, bool isImportant);
    FOR_EACH_STYLE_ATTRIBUTE(DECLARE_ATTRIBUTE_SETTER)
#undef DECLARE_ATTRIBUTE_SETTER

#define DECLARE_ATTRIBUTE_GETTER_FOURSIDE(PRE, ...) \
    String* PRE##__VA_ARGS__(bool* isCombined = nullptr);

    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Margin);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Padding);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Border, Width);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Border, Style);
    DECLARE_ATTRIBUTE_GETTER_FOURSIDE(Border, Color);
#undef DECLARE_ATTRIBUTE_GETTER_FOURSIDE

    static String* combineBoxString(String* t, String* r, String* b, String* l,
                                    bool* isCombined = nullptr);

    uint32_t length() const;

    String* item(uint32_t index);

    String* cssText() const;
    void setCssText(String* text);

    String* getPropertyValue(String* name);
    void setProperty(String* name, String* value, String* priority);

protected:
    GCVector<CSSStyleValuePair> m_cssValues;
    Element* m_element;
    StyleType m_styleType;
};
}

#endif
