/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_AUDIO_WORKLET)

#ifndef __StarfishWorklet__
#define __StarfishWorklet__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class Promise;

// https://html.spec.whatwg.org/multipage/worklets.html#dictdef-workletoptions
struct WorkletOptions {
    DEFINE_GETTER_SETTER(String*, credentials, Credentials)
    String* m_credentials{ nullptr };
};

// https://html.spec.whatwg.org/multipage/worklets.html#worklets-worklet
class Worklet : public ScriptWrappable {
public:
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWorklet() const override;

    // The credentials option is not used: worklet modules are fetched with
    // the default same-origin credentials mode.
    virtual Promise* addModule(String* moduleURL,
                               WorkletOptions options = WorkletOptions()) = 0;

protected:
    Worklet()
        : ScriptWrappable(this)
    {
    }
};
} // namespace Starfish

#endif
#endif
