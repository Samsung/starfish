#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "extra/SourceBuffer.h"

namespace StarFish {

using namespace escargot;

ESValue appendBufferSourceBufferFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, SourceBuffer);
    SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

    try {
        if (false) {
        }
#ifdef USE_ES6_FEATURE
        else if (firstArg.isESPointer() &&
                 firstArg.asESPointer()->isESArrayBufferObject()) {
            ESArrayBufferObject* v =
                firstArg.asESPointer()->asESArrayBufferObject();
            sourceBuffer->appendBuffer((uint8_t*)v->data(), v->bytelength());
        } else if (firstArg.isESPointer() &&
                   firstArg.asESPointer()->isESArrayBufferView()) {
            ESArrayBufferView* v =
                firstArg.asESPointer()->asESArrayBufferView();
            uint8_t* p = (uint8_t*)v->buffer()->data();
            sourceBuffer->appendBuffer(p, v->bytelength());
        }
#endif
        else {
            COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "appendBuffer",
                            "SourceBuffer", SIGNATURE_NOT_FOUND);
            THROW_EXCEPTION(msg);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return ESValue(ESValue::ESUndefined);
}
}
#endif
