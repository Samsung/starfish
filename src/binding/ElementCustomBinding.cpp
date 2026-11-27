/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "core/animation/TimingOptions.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/style/StyleRule.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

extern KeyframeAnimationOptions toKeyframeAnimationOptionsFromValueRef(
    ExecutionStateRef*, ValueRef*);

namespace {

    // Web IDL reads @@iterator and the iterator's next method once. A missing
    // method selects the scalar (or property-indexed keyframe) alternative.
    template <typename Callback>
    bool readSequence(ExecutionStateRef* state, ValueRef* value,
                      Callback consume)
    {
        if (!value->isObject()) {
            return false;
        }
        auto method = value->asObject()->get(
            state, state->context()->vmInstance()->iteratorSymbol());
        if (method->isUndefinedOrNull()) {
            return false;
        }
        auto iterator = method->call(state, value, 0, nullptr);
        if (!iterator->isObject()) {
            THROW_EXCEPTION("Expected an iterator object");
        }
        auto next = iterator->asObject()->get(
            state, StringRef::createFromASCII("next"));
        while (true) {
            auto step = next->call(state, iterator, 0, nullptr);
            if (!step->isObject()) {
                THROW_EXCEPTION("Expected an iterator result object");
            }
            if (step->asObject()
                    ->get(state, StringRef::createFromASCII("done"))
                    ->toBoolean(state)) {
                return true;
            }
            consume(step->asObject()->get(state,
                                          StringRef::createFromASCII("value")));
        }
    }

    template <typename Callback>
    void readMember(ExecutionStateRef* state, ValueRef* value, bool allowList,
                    Callback consume)
    {
        if (!allowList || !readSequence(state, value, consume)) {
            consume(value);
        }
    }

    using Key = CSSStyleValuePair::KeyKind;

    void setProperty(CSSStyleDeclaration* declaration, Key key, String* value)
    {
        auto utf8 = value->toUTF8NonGCString();
        declaration->setPropertyInternal(key, utf8.data(), utf8.size(), false);
    }

    // https://www.w3.org/TR/web-animations-1/#processing-a-keyframes-argument
    // Dictionary members include inherited properties. Only enumerable own CSS
    // properties are read, in sorted order, after the dictionary members.
    struct KeyframeInput {
        GCVector<CSSStyleDeclaration*> frames;
        GCAtomicVector<Optional<double>> offsets;
        GCVector<String*> easings;

        void read(ExecutionStateRef* state, Document* document, ValueRef* item,
                  bool sequence)
        {
            CSSStyleDeclaration* frame = nullptr;
            if (sequence) {
                frame = new CSSStyleDeclaration(document);
                frames.push_back(frame);
            }
            if (item->isUndefinedOrNull()) {
                offsets.emplace_back();
                easings.push_back(nullptr);
                return;
            }
            if (!item->isObject()) {
                THROW_EXCEPTION("Expected a keyframe object");
            }
            auto object = item->asObject();
            auto composite =
                object->get(state, StringRef::createFromASCII("composite"));
            if (!composite->isUndefined()) {
                readMember(state, composite, !sequence, [&](ValueRef* v) {
                    auto name = toBrowserString(state, v);
                    if (!(name->equals("auto") || name->equals("replace") ||
                          name->equals("add") || name->equals("accumulate"))) {
                        THROW_EXCEPTION("Invalid keyframe composite operation");
                    }
                });
            }
            auto easing =
                object->get(state, StringRef::createFromASCII("easing"));
            if (!easing->isUndefined()) {
                readMember(state, easing, !sequence, [&](ValueRef* v) {
                    easings.push_back(toBrowserString(state, v));
                });
            } else if (sequence) {
                easings.push_back(nullptr);
            }
            auto offset =
                object->get(state, StringRef::createFromASCII("offset"));
            if (!offset->isUndefined()) {
                readMember(state, offset, !sequence, [&](ValueRef* v) {
                    Optional<double> number;
                    if (!v->isUndefinedOrNull()) {
                        double d = v->toNumber(state);
                        if (!std::isfinite(d)) {
                            THROW_EXCEPTION("Keyframe offsets must be finite");
                        }
                        number = d;
                    }
                    offsets.push_back(number);
                });
            } else if (sequence) {
                offsets.emplace_back();
            }

            GCVector<String*> properties;
            object->enumerateObjectOwnProperties(
                state, [&](ExecutionStateRef* st, ValueRef* name, bool,
                           bool enumerable, bool) {
                    if (enumerable && name->isString()) {
                        properties.push_back(toBrowserString(st, name));
                    }
                    return true;
                });
            std::sort(properties.begin(), properties.end(),
                      [](String* a, String* b) { return a->compare(b) < 0; });

            GCUnorderedMap<double, CSSStyleDeclaration*> indexedFrames;
            for (auto property : properties) {
                if (property->equals("offset") || property->equals("easing") ||
                    property->equals("composite")) {
                    continue;
                }
                auto name = property->toUTF8NonGCString();
                Key key = CSSStyleLookupTrie::lookupCSSStyleCamelCase(
                    name.data(), name.size());
                if (key == Key::Unknown ||
                    key == Key::AnimationTimingFunction) {
                    continue;
                }
                auto value = object->get(state, toJSString(property));
                if (sequence) {
                    setProperty(frame, key, toBrowserString(state, value));
                    continue;
                }
                GCVector<String*> values;
                readMember(state, value, true, [&](ValueRef* v) {
                    values.push_back(toBrowserString(state, v));
                });
                for (size_t i = 0; i < values.size(); ++i) {
                    double position = values.size() == 1
                                          ? 1.0
                                          : double(i) / (values.size() - 1);
                    auto& declaration = indexedFrames[position];
                    if (!declaration) {
                        declaration = new CSSStyleDeclaration(document);
                    }
                    setProperty(declaration, key, values[i]);
                }
            }
            if (!sequence) {
                GCAtomicVector<double> positions;
                for (const auto& entry : indexedFrames) {
                    positions.push_back(entry.first);
                }
                std::sort(positions.begin(), positions.end());
                for (double position : positions) {
                    frames.push_back(indexedFrames.at(position));
                }
            }
        }

        void makeRules(ExecutionStateRef* state,
                       GCVector<StyleRuleBase*>& rules)
        {
            offsets.resize(frames.size());
            double previous = 0;
            for (auto offset : offsets) {
                if (offset) {
                    if (offset.value() < previous || offset.value() > 1) {
                        THROW_EXCEPTION(
                            "Keyframe offsets must be ordered and within [0, "
                            "1]");
                    }
                    previous = offset.value();
                }
            }
            // Even easings beyond the last generated property-indexed frame
            // must parse. Defer parsing until all observable getters have run.
            GCVector<ValueList*> timingLists;
            for (auto easing : easings) {
                ValueList* list = nullptr;
                if (easing) {
                    auto timing = TimingOptions::parseEasing(easing);
                    if (!timing) {
                        THROW_EXCEPTION("Invalid keyframe easing");
                    }
                    if (timingLists.size() < frames.size()) {
                        CSSStyleValuePair value;
                        value.setTimingFunctionPointerValue(timing.value());
                        list = new ValueList(Separator::CommaSeparator);
                        list->push_back(value);
                    }
                }
                timingLists.push_back(list);
            }
            if (frames.empty()) {
                return;
            }
            if (!offsets.back()) {
                offsets.back() = 1.0;
            }
            if (!offsets[0]) {
                offsets[0] = 0.0;
            }
            size_t start = 0;
            for (size_t end = 1; end < offsets.size(); ++end) {
                if (offsets[end]) {
                    for (size_t i = start + 1; i < end; ++i) {
                        offsets[i] =
                            offsets[start].value() +
                            (offsets[end].value() - offsets[start].value()) *
                                (double(i - start) / (end - start));
                    }
                    start = end;
                }
            }
            for (size_t i = 0; i < frames.size(); ++i) {
                if (frames[i]->length()) {
                    if (!timingLists.empty()) {
                        auto list = timingLists[i % timingLists.size()];
                        if (list) {
                            CSSStyleValuePair value;
                            value.setValueList(list);
                            frames[i]->addCSSValuePair(
                                Key::AnimationTimingFunction, value);
                        }
                    }
                    GCAtomicVector<double> selectors;
                    selectors.push_back(offsets[i].value());
                    rules.push_back(
                        new StyleRuleKeyframe(selectors, frames[i]));
                }
            }
        }
    };
} // namespace

ValueRef* animateElementFunction(ExecutionStateRef* state, ValueRef* thisValue,
                                 size_t argc, ValueRef** argv, bool)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    if (!argc || !(argv[0]->isUndefinedOrNull() || argv[0]->isObject())) {
        THROW_EXCEPTION("Expected a nullable keyframe object");
    }
    KeyframeAnimationOptions options;
    if (argc > 1 && !argv[1]->isUndefinedOrNull()) {
        if (argv[1]->isObject()) {
            options = toKeyframeAnimationOptionsFromValueRef(state, argv[1]);
        } else {
            options.setDuration(argv[1]->toNumber(state));
        }
    }
    auto context = fetchExecutionContext(state->context());
    TimingFunction* easing;
    try {
        easing = TimingOptions::validate(context, options);
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    KeyframeInput input;
    if (argv[0]->isObject()) {
        auto document = originalObj->document();
        if (!readSequence(state, argv[0], [&](ValueRef* item) {
                input.read(state, document, item, true);
            })) {
            input.read(state, document, argv[0], false);
        }
    }
    GCVector<StyleRuleBase*> rules;
    input.makeRules(state, rules);
    return originalObj->animate(context, rules, options, easing)->scriptValue();
}
} // namespace Starfish
