/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "AdoptedStyleSheets.h"

#include "binding/ObservableArray.h"
#include "core/dom/Document.h"
#include "core/dom/ShadowRoot.h"
#include "core/page/BrowsingContext.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/Style.h"

#include <EscargotPublic.h>

using namespace Escargot;

namespace Starfish {

namespace {

    GCVector<CSSStyleSheet*>& backingListOf(ScriptWrappable* host)
    {
        Node* node = static_cast<Node*>(host);
        if (node->isDocument()) {
            return static_cast<Document*>(node)
                ->adoptedStyleSheetsBackingList();
        }
        return static_cast<ShadowRoot*>(node)->adoptedStyleSheetsBackingList();
    }

    StyleResolver& resolverOf(Node* node)
    {
        if (node->isDocument()) {
            return static_cast<Document*>(node)->styleResolver();
        }
        return static_cast<ShadowRoot*>(node)->styleResolver();
    }

    // Push the current backing list into the host's style resolver and request
    // a restyle. There is no DOM mutation to drive the cascade as with <style>
    // insertion, so the affected scope is marked dirty explicitly: a
    // document's adopted sheets apply document-wide, while a shadow root's
    // apply only within its host's flat tree.
    void syncAdoptedSheetsToCascade(ScriptWrappable* host)
    {
        Node* node = static_cast<Node*>(host);
        resolverOf(node).setAdoptedSheets(backingListOf(host));

        if (node->isDocument()) {
            node->document()
                ->browsingContext()
                ->setWholeDocumentNeedsStyleRecalc();
            node->document()->setNeedsFrameTreeBuildWithoutSelf();
            return;
        }

        // Shadow root: restyle the host's flat tree, matching StyleResolver's
        // own reasoning for a `<style>`/`<link>` sheet change (see
        // invalidateShadowScopeForSheetChange()) -- adopted `:host`/`::slotted`
        // rules reach the same three targets (host, shadow content, and
        // slotted light-DOM children) via the same promotion path.
        ShadowRoot* shadowRoot = static_cast<ShadowRoot*>(node);
        if (shadowRoot->host() != nullptr) {
            shadowRoot->styleResolver().invalidateShadowScopeForSheetChange();
        }
    }

    // Returns the CSSStyleSheet wrapped by `value`, or nullptr if `value` is
    // not a CSSStyleSheet wrapper.
    CSSStyleSheet* toCSSStyleSheet(ValueRef* value)
    {
        if (value->isObject() && value->asObject()->extraData() != nullptr &&
            ((ScriptWrappable*)value->asObject()->extraData())
                ->isCSSStyleSheet()) {
            return (CSSStyleSheet*)value->asObject()->extraData();
        }
        return nullptr;
    }

    void throwNotACSSStyleSheet(ExecutionStateRef* state)
    {
        state->throwException(ErrorObjectRef::create(
            state, ErrorObjectRef::TypeError,
            StringRef::createFromASCII(
                "Failed to set adoptedStyleSheets: the provided value is not a "
                "CSSStyleSheet.")));
    }

    bool setIndexedValue(ExecutionStateRef* state, ScriptWrappable* host,
                         uint32_t index, ValueRef* value)
    {
        CSSStyleSheet* sheet = toCSSStyleSheet(value);
        if (sheet == nullptr) {
            throwNotACSSStyleSheet(state);
            return false;
        }
        GCVector<CSSStyleSheet*>& backing = backingListOf(host);
        if (index < backing.size()) {
            backing[index] = sheet;
        } else if (index == backing.size()) {
            backing.push_back(sheet);
        } else {
            return false;
        }
        syncAdoptedSheetsToCascade(host);
        return true;
    }

    bool deleteIndexedValue(ExecutionStateRef* state, ScriptWrappable* host,
                            uint32_t index)
    {
        GCVector<CSSStyleSheet*>& backing = backingListOf(host);
        if (index < backing.size()) {
            if (index == backing.size() - 1) {
                backing.pop_back();
            } else {
                // A genuine `delete arr[i]` leaves a hole; store it as null.
                backing[index] = nullptr;
            }
            syncAdoptedSheetsToCascade(host);
        }
        return true;
    }

    ValueRef* getIndexedValue(ExecutionStateRef* state, ScriptWrappable* host,
                              uint32_t index)
    {
        GCVector<CSSStyleSheet*>& backing = backingListOf(host);
        if (index < backing.size() && backing[index] != nullptr) {
            return backing[index]->scriptValue();
        }
        return ValueRef::createUndefined();
    }

    uint32_t lengthOf(ScriptWrappable* host)
    {
        return (uint32_t)backingListOf(host).size();
    }

    bool setLength(ExecutionStateRef* state, ScriptWrappable* host,
                   uint32_t newLength)
    {
        GCVector<CSSStyleSheet*>& backing = backingListOf(host);
        if (newLength < backing.size()) {
            backing.resize(newLength);
            syncAdoptedSheetsToCascade(host);
        }
        return true;
    }

    const ObservableArrayCallbacks s_callbacks = { setIndexedValue,
                                                   deleteIndexedValue,
                                                   getIndexedValue, lengthOf,
                                                   setLength };

    ScriptProxyObject& proxySlotOf(Node* host)
    {
        if (host->isDocument()) {
            return static_cast<Document*>(host)->adoptedStyleSheetsProxySlot();
        }
        return static_cast<ShadowRoot*>(host)->adoptedStyleSheetsProxySlot();
    }

} // namespace

namespace AdoptedStyleSheets {

    ScriptProxyObject observableArray(ExecutionStateRef* state, Node* host)
    {
        ScriptProxyObject& slot = proxySlotOf(host);
        if (slot == nullptr) {
            slot = ObservableArray::create(state, host, &s_callbacks);
        }
        return slot;
    }

    void setFromObservableArray(ExecutionStateRef* state, Node* host,
                                ValueRef* value)
    {
        if (!value->isObject()) {
            throwNotACSSStyleSheet(state);
            return;
        }

        // Build the new contents into a temp first, validating every element,
        // so a mid-iteration type error leaves the existing backing list
        // untouched.
        ObjectRef* array = value->asObject();
        uint32_t length =
            array->get(state, StringRef::createFromASCII("length"))
                ->toUint32(state);
        GCVector<CSSStyleSheet*> next;
        for (uint32_t i = 0; i < length; i++) {
            CSSStyleSheet* sheet = toCSSStyleSheet(
                array->getIndexedProperty(state, ValueRef::create(i)));
            if (sheet == nullptr) {
                throwNotACSSStyleSheet(state);
                return;
            }
            next.push_back(sheet);
        }

        GCVector<CSSStyleSheet*>& backing = backingListOf(host);
        backing.clear();
        for (uint32_t i = 0; i < next.size(); i++) {
            backing.push_back(next[i]);
        }

        syncAdoptedSheetsToCascade(host);

        ObservableArray::syncFromHost(state, observableArray(state, host));
    }

} // namespace AdoptedStyleSheets

} // namespace Starfish
