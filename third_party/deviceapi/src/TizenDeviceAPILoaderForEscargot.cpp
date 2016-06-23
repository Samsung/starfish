#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"

#include "Escargot.h"

#include <dlfcn.h>
#include "ExtensionAdapter.h"
#include "ExtensionManager.h"

#include "StarFishConfig.h"
#include "platform/window/Window.h"
#include "dom/binding/ScriptBindingInstance.h"

using namespace escargot;

namespace DeviceAPI {

TizenStrings::TizenStrings(escargot::ESVMInstance* instance)
    : m_instance(instance)
    , m_initialized(false)
{
    initializeEarlyStrings();
}

#define INIT_TIZEN_STRING(name) name = escargot::InternalAtomicString(m_instance, "" #name);
void TizenStrings::initializeEarlyStrings()
{
    DEVICEAPI_LOG_INFO("Enter");

    FOR_EACH_EARLY_TIZEN_STRINGS(INIT_TIZEN_STRING)
}

void TizenStrings::initializeLazyStrings()
{
    DEVICEAPI_LOG_INFO("Enter");

    if (m_initialized)
        return;

    FOR_EACH_LAZY_TIZEN_STRINGS(INIT_TIZEN_STRING)

    m_apis.push_back(application);
    m_apis.push_back(filesystem);
    m_apis.push_back(messageport);
    m_apis.push_back(systeminfo);
    m_apis.push_back(sensorservice);
    m_apis.push_back(preference);

    // FIXME: this should be automated
    m_entryPoints[ApplicationControl.string()] = application;
    m_entryPoints[ApplicationControlData.string()] = application;

    m_initialized = true;
}
#undef INIT_TIZEN_STRING

void printArguments(escargot::ESVMInstance* instance)
{
    size_t count = instance->currentExecutionContext()->argumentCount();
    DEVICEAPI_LOG_INFO("printing %u arguments", count);
    for (size_t i = 0; i < count; i++) {
        DEVICEAPI_LOG_INFO("argument %u : %s", i, instance->currentExecutionContext()->arguments()[i].toString()->utf8Data());
    }
}

wrt::xwalk::Extension* ExtensionManagerInstance::getExtension(const char* apiName)
{
    DEVICEAPI_LOG_INFO("Enter");
    wrt::xwalk::ExtensionMap& extensions = wrt::xwalk::ExtensionManager::GetInstance()->extensions();

    auto it = extensions.find(apiName);
    if (it == extensions.end()) {
        DEVICEAPI_LOG_INFO("Enter");
        char library_path[512];
        snprintf(library_path, 512, "/usr/lib/wrt-plugins-widget/lib%s.so", apiName);
        wrt::xwalk::Extension* extension = new wrt::xwalk::Extension(library_path, nullptr);
        if (extension->Initialize()) {
            wrt::xwalk::ExtensionManager::GetInstance()->RegisterExtension(extension);
            extensions[apiName] = extension;
            return extension;
        } else {
            DEVICEAPI_LOG_INFO("Cannot initialize extension %s", apiName);
            return nullptr;
        }
    } else {
        return it->second;
    }
}

escargot::ESObject* ExtensionManagerInstance::initializeExtensionInstance(const char* apiName)
{
    DEVICEAPI_LOG_INFO("Enter");

    wrt::xwalk::Extension* extension = getExtension(apiName);
    if (!extension) {
        DEVICEAPI_LOG_INFO("Cannot load extension %s", apiName);
        return ESObject::create();
    }
    escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();

    std::jmp_buf tryPosition;
    escargot::ESObject* ret;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        escargot::ESFunctionObject* initializer = instance->evaluate(escargot::ESString::create(extension->javascript_api().c_str())).asESPointer()->asESFunctionObject();
        escargot::ESObject* extensionObject = createExtensionObject();
        wrt::xwalk::ExtensionInstance* extensionInstance = extension->CreateInstance();
        m_extensionInstances[extensionObject] = extensionInstance;
        escargot::ESValue arguments[] = { extensionObject };
        ret = escargot::ESFunctionObject::call(instance, initializer, escargot::ESValue(), arguments, 1, false).asESPointer()->asESObject();
        m_instance->unregisterTryPos(&tryPosition);
    } else {
        escargot::ESValue err = instance->getCatchedError();
        DEVICEAPI_LOG_ERROR("Uncaught %s\n", err.toString()->utf8Data());
    }

    return ret;
}

escargot::ESObject* ExtensionManagerInstance::createExtensionObject()
{
    DEVICEAPI_LOG_INFO("Enter");

    escargot::ESObject* extensionObject = ESObject::create();

    extensionObject->defineDataProperty(m_strings->postMessage.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_ERROR("extension.postMessage UNIMPLEMENTED");
            printArguments(instance);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return ESValue();
        }, m_strings->postMessage.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->sendSyncMessage.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_INFO("extension.sendSyncMessage");
            printArguments(instance);

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceGet(instance);
            wrt::xwalk::ExtensionInstance* extensionInstance = extensionManagerInstance->getExtensionInstanceFromCallingContext(instance);
            if (!extensionInstance || (instance->currentExecutionContext()->argumentCount() != 1))
                return ESValue(false);

            escargot::ESString* message = instance->currentExecutionContext()->readArgument(0).toString();
            extensionInstance->HandleSyncMessage(message->utf8Data());

            std::string reply = extensionInstance->sync_replay_msg();
            DEVICEAPI_LOG_INFO("extension.sendSyncMessage Done with reply %s", reply.c_str());

            if (reply.empty())
                return escargot::ESValue(escargot::ESValue::ESNullTag::ESNull);
            return escargot::ESString::create(reply.c_str());
        }, m_strings->sendSyncMessage.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->sendSyncData.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_INFO("extension.sendSyncData");
            printArguments(instance);

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceGet(instance);
            wrt::xwalk::ExtensionInstance* extensionInstance = extensionManagerInstance->getExtensionInstanceFromCallingContext(instance);
            if (!extensionInstance || (instance->currentExecutionContext()->argumentCount() < 1))
                return ESValue(false);

            ChunkData chunkData(nullptr, 0);
            if (instance->currentExecutionContext()->argumentCount() > 1) {
                escargot::ESValue dataValue = instance->currentExecutionContext()->readArgument(1);
                if (dataValue.isESPointer()) {
                    if (dataValue.asESPointer()->isESObject()) {
                        escargot::ESObject* arrayData = dataValue.asESPointer()->asESObject();
                        size_t length = arrayData->get(escargot::strings->length.string()).toLength();
                        uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t) * length);
                        for (size_t i = 0; i < length; i++) {
                            buffer[i] = static_cast<uint8_t>(arrayData->get(ESValue(i)).toNumber());
                        }
                        chunkData = ChunkData(buffer, length);
                    } else if (dataValue.asESPointer()->isESString()) {
                        escargot::NullableUTF8String stringData = dataValue.toString()->toNullableUTF8String();
                        chunkData = ChunkData((uint8_t*)stringData.m_buffer, stringData.m_bufferSize);
                    }
                }
            }
            escargot::ESString* message = instance->currentExecutionContext()->readArgument(0).toString();
            extensionInstance->HandleSyncData(message->utf8Data(), chunkData.m_buffer, chunkData.m_length);

            uint8_t* replyBuffer = nullptr;
            size_t replyLength = 0;
            std::string reply = extensionInstance->sync_data_reply_msg(&replyBuffer, &replyLength);

            DEVICEAPI_LOG_INFO("extension.sendSyncData Done with reply %s (buffer %s)", reply.c_str(), replyBuffer);

            if (reply.empty())
                return escargot::ESValue(escargot::ESValue::ESNullTag::ESNull);

            escargot::ESObject* returnObject = escargot::ESObject::create();
            returnObject->defineDataProperty(extensionManagerInstance->strings()->reply.string(), true, true, true, escargot::ESString::create(reply.c_str()));
            if (replyBuffer || replyLength > 0) {
                size_t chunkID = extensionManagerInstance->addChunk(replyBuffer, replyLength);
                returnObject->defineDataProperty(extensionManagerInstance->strings()->chunk_id.string(), true, true, true, ESValue(chunkID));
            }

            return returnObject;
        }, m_strings->sendSyncData.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->sendRuntimeMessage.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_ERROR("extension.sendRuntimeMessage UNIMPLEMENTED");
            printArguments(instance);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return ESValue();
        }, m_strings->sendRuntimeMessage.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->sendRuntimeAsyncMessage.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_ERROR("extension.sendRuntimeAsyncMessage UNIMPLEMENTED");
            printArguments(instance);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return ESValue();
        }, m_strings->sendRuntimeAsyncMessage.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->sendRuntimeSyncMessage.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_ERROR("extension.sendRuntimeSyncMessage UNIMPLEMENTED");
            printArguments(instance);
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            return ESValue();
        }, m_strings->sendRuntimeSyncMessage.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->setMessageListener.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_INFO("extension.setMessageListener");
            printArguments(instance);

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceGet(instance);
            wrt::xwalk::ExtensionInstance* extensionInstance = extensionManagerInstance->getExtensionInstanceFromCallingContext(instance);
            if (!extensionInstance || instance->currentExecutionContext()->argumentCount() != 1)
                return ESValue(false);

            escargot::ESValue listenerValue = instance->currentExecutionContext()->readArgument(0);
            if (listenerValue.isUndefined()) {
                extensionInstance->set_post_message_listener(nullptr);
                return ESValue(true);
            }
            if (!listenerValue.isESPointer() || !listenerValue.asESPointer()->isESFunctionObject()) {
                DEVICEAPI_LOG_ERROR("Trying to set message listener with invalid value.");
                return ESValue(false);
            }

            escargot::ESFunctionObject* listener = listenerValue.asESPointer()->asESFunctionObject();
            ESPostMessageListener* postMessageListener = ESPostMessageListener::create(instance, listener);
            extensionInstance->set_post_message_listener(postMessageListener);

            extensionManagerInstance->m_postListeners.push_back(postMessageListener);

            return ESValue(true);
        }, m_strings->setMessageListener.string(), 0, true, true)
    );
    extensionObject->defineDataProperty(m_strings->receiveChunkData.string(), true, true, true,
        escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            DEVICEAPI_LOG_INFO("extension.receiveChunkData");
            printArguments(instance);

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceGet(instance);
            wrt::xwalk::ExtensionInstance* extensionInstance = extensionManagerInstance->getExtensionInstanceFromCallingContext(instance);
            if (!extensionInstance || (instance->currentExecutionContext()->argumentCount() < 1))
                return ESValue(false);

            TizenStrings* strings = extensionManagerInstance->strings();

            size_t chunkID = instance->currentExecutionContext()->readArgument(0).toNumber();
            ExtensionManagerInstance::ChunkData chunkData = extensionManagerInstance->getChunk(chunkID);
            if (!chunkData.m_buffer)
                return escargot::ESValue(escargot::ESValue::ESNullTag::ESNull);

            escargot::ESString* type = instance->currentExecutionContext()->readArgument(1).toString();
            bool isStringType = (*type != *strings->octet.string());

            escargot::ESValue ret;
            if (isStringType) {
                ret = escargot::ESString::create((const char*)chunkData.m_buffer);
            } else {
                escargot::ESArrayObject* octetArray = escargot::ESArrayObject::create(chunkData.m_length);
                for (size_t i = 0; i < chunkData.m_length; i++) {
                    octetArray->set(i, ESValue(chunkData.m_buffer[i]));
                }
                ret = octetArray;
            }
            free(chunkData.m_buffer);
            return ret;
        }, m_strings->receiveChunkData.string(), 0, true, true)
    );

    return extensionObject;
}

wrt::xwalk::ExtensionInstance* ExtensionManagerInstance::getExtensionInstanceFromCallingContext(escargot::ESVMInstance* instance)
{
    escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
    if (thisValue.isUndefinedOrNull())
        return nullptr;

    auto it = m_extensionInstances.find(thisValue.asESPointer());
    if (it == m_extensionInstances.end())
        return nullptr;

    return it->second;
}

size_t ExtensionManagerInstance::addChunk(uint8_t* buffer, size_t length)
{
    DEVICEAPI_LOG_INFO("Enter");
    size_t chunkID = m_chunkID++;
    m_chunkDataMap[chunkID] = ChunkData(buffer, length);
    return chunkID;
}

ExtensionManagerInstance::ChunkData ExtensionManagerInstance::getChunk(size_t chunkID)
{
    DEVICEAPI_LOG_INFO("Enter");
    auto it = m_chunkDataMap.find(chunkID);
    if (it == m_chunkDataMap.end()) {
        return ChunkData(nullptr, 0);
    } else {
        ChunkData chunkData = it->second;
        m_chunkDataMap.erase(it);
        return chunkData;
    }
}

ExtensionManagerInstance::ExtensionManagerInstanceMap ExtensionManagerInstance::s_extensionManagerInstances;

ExtensionManagerInstance::ExtensionManagerInstance(escargot::ESVMInstance* instance)
    : m_instance(instance)
    , m_chunkID(0)
{
    DEVICEAPI_LOG_INFO("new ExtensionManagerInstance %p", this);
    m_strings = new TizenStrings(m_instance);

    m_instance->globalObject()->defineAccessorProperty(m_strings->tizen.string(),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {

            DEVICEAPI_LOG_INFO("Enter");

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
            TizenStrings* strings = extensionManagerInstance->strings();
            strings->initializeLazyStrings();

            // initialize tizen object
            escargot::ESObject* tizenObject = extensionManagerInstance->initializeExtensionInstance("tizen");
            for (auto api : strings->apis()) {
                tizenObject->defineAccessorProperty(api.string(),
                    [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
                        DEVICEAPI_LOG_INFO("Loading plugin for %s API", name->utf8Data());
                        ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
                        escargot::ESObject* apiObject = extensionManagerInstance->initializeExtensionInstance(name->utf8Data());
                        obj->defineDataProperty(name, false, true, false, apiObject, true);
                        return apiObject;
                    }, nullptr, false, true, false);
            }

            for (auto entryPoint : strings->entryPoints()) {
                tizenObject->defineAccessorProperty(entryPoint.first,
                    [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
                        ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
                        TizenStrings* strings = extensionManagerInstance->strings();
                        obj->deleteProperty(name, true);
                        obj->get(strings->entryPoints().find(name)->second.string()); // trigger plugin loading
                        escargot::ESValue ret = obj->get(name);
                        obj->defineDataProperty(name, true, true, true, ret, true);
                        return ret;
                    }, [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name, const escargot::ESValue& value) -> void {
                        obj->defineDataProperty(name, true, true, true, value, true);
                    }, true, true, true);
            }

            // re-define tizen object
            obj->defineDataProperty(strings->tizen.string(), false, true, false, tizenObject, true);

            return tizenObject;
        }, nullptr, false, true, false);

    m_instance->globalObject()->defineAccessorProperty(m_strings->xwalk.string(),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {

            DEVICEAPI_LOG_INFO("Enter");

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
            TizenStrings* strings = extensionManagerInstance->strings();
            strings->initializeLazyStrings();

            // initialize xwalk object
            escargot::ESObject* xwalkObject = ESObject::create();
            xwalkObject->defineAccessorProperty(strings->utils.string(),
                [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
                    DEVICEAPI_LOG_INFO("Loading plugin for xwalk.utils");
                    ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
                    escargot::ESObject* utilsObject = extensionManagerInstance->initializeExtensionInstance("utils");
                    obj->defineDataProperty(name, false, true, false, utilsObject, true);
                    return utilsObject;
                }, nullptr, false, true, false);

            // re-define xwalk object
            obj->defineDataProperty(strings->xwalk.string(), false, true, false, xwalkObject, true);

            return xwalkObject;
        }, nullptr, false, true, false);

    m_instance->globalObject()->defineAccessorProperty(m_strings->webapis.string(),
        [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {

            DEVICEAPI_LOG_INFO("Enter");

            ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
            TizenStrings* strings = extensionManagerInstance->strings();
            strings->initializeLazyStrings();

            // initialize webapis object
            escargot::ESObject* webapisObject = ESObject::create();
            webapisObject->defineAccessorProperty(strings->sa.string(),
                [](::escargot::ESObject* obj, ::escargot::ESObject* originalObj, escargot::ESString* name) -> escargot::ESValue {
                    DEVICEAPI_LOG_INFO("Loading plugin for Samsung Accessory protocol API");
                    ExtensionManagerInstance* extensionManagerInstance = ExtensionManagerInstanceCurrentInstance();
                    escargot::ESObject* saObject = extensionManagerInstance->initializeExtensionInstance("sa");
                    obj->defineDataProperty(name, false, true, false, saObject, true);
                    return saObject;
                }, nullptr, false, true, false);

            // re-define webapis object
            obj->defineDataProperty(strings->webapis.string(), false, true, false, webapisObject, true);

            return webapisObject;
        }, nullptr, false, true, false);

    s_extensionManagerInstances[m_instance] = this;
    DEVICEAPI_LOG_INFO("%zu => %zu", s_extensionManagerInstances.size() - 1, s_extensionManagerInstances.size());
}

ExtensionManagerInstance::~ExtensionManagerInstance()
{
    DEVICEAPI_LOG_INFO("delete ExtensionManagerInstance %p", this);
    for (auto it : m_extensionInstances)
        delete it.second;
    for (auto it : m_postListeners)
        it->finalize();
    auto it = s_extensionManagerInstances.find(m_instance);
    s_extensionManagerInstances.erase(it);
    DEVICEAPI_LOG_INFO("%zu => %zu", s_extensionManagerInstances.size() + 1, s_extensionManagerInstances.size());
}

ExtensionManagerInstance* ExtensionManagerInstance::get(escargot::ESVMInstance* instance)
{
    auto it = s_extensionManagerInstances.find(instance);
    if (it == s_extensionManagerInstances.end())
        return nullptr;
    else
        return it->second;
}

ExtensionManagerInstance* ExtensionManagerInstance::currentInstance()
{
    return get(escargot::ESVMInstance::currentInstance());
}

void initialize(escargot::ESVMInstance* instance)
{
    DEVICEAPI_LOG_INFO("Enter with instance %p", instance);
    new ExtensionManagerInstance(instance);
}

void close(escargot::ESVMInstance* instance)
{
    DEVICEAPI_LOG_INFO("Enter with instance %p", instance);
    delete ExtensionManagerInstance::get(instance);
}

}

#endif
