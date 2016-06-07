#ifndef __TizenDeviceAPILoaderForEscargot__
#define __TizenDeviceAPILoaderForEscargot__

#ifdef TIZEN_DEVICE_API

#include "StarFishConfig.h"
#include "Escargot.h"

#undef LOGGER_TAG
#define LOGGER_TAG "StarFishDeviceAPI"

#ifndef __MODULE__
#define __MODULE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define _LOGGER_LOG(prio, fmt, args...) \
    dlog_print(prio, LOGGER_TAG, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##args); \

#define _LOGGER_SLOG(prio, fmt, args...) \
    dlog_print(prio, LOGGER_TAG, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##args); \

#define DEVICEAPI_LOG_INFO(fmt, args...) _LOGGER_LOG(DLOG_INFO, fmt, ##args)
#define DEVICEAPI_LOG_ERROR(fmt, args...) _LOGGER_LOG(DLOG_ERROR, fmt, ##args)
#define DEVICEAPI_LOG_WARN(fmt, args...) _LOGGER_LOG(DLOG_WARN, fmt, ##args)

#define DEVICEAPI_SLOG_INFO(fmt, args...) _LOGGER_SLOG(DLOG_INFO, fmt, ##args)
#define DEVICEAPI_SLOG_ERROR(fmt, args...) _LOGGER_SLOG(DLOG_ERROR, fmt, ##args)
#define DEVICEAPI_SLOG_WARN(fmt, args...) _LOGGER_SLOG(DLOG_WARN, fmt, ##args)

namespace wrt {
namespace xwalk {
class Extension;
class ExtensionInstance;
}
}

namespace DeviceAPI {

class ESPostListener;

#define FOR_EACH_EARLY_TIZEN_STRINGS(F) \
    F(tizen) \
    F(xwalk) \
    F(webapis) \

#define FOR_EACH_LAZY_TIZEN_STRINGS(F) \
    F(utils) \
    F(sa) \
    F(messageport) \
    F(sensorservice) \
    F(filesystem) \
    F(application) \
    F(systeminfo) \
    F(preference) \
    F(common) \
    F(ApplicationControl) \
    F(ApplicationControlData) \
    F(extension) \
    F(postMessage) \
    F(sendSyncMessage) \
    F(sendSyncData) \
    F(sendRuntimeMessage) \
    F(sendRuntimeSyncMessage) \
    F(sendRuntimeAsyncMessage) \
    F(setMessageListener) \
    F(receiveChunkData) \
    F(reply) \
    F(chunk_id) \
    F(string) \
    F(octet) \

class TizenStrings {
public:
    TizenStrings(escargot::ESVMInstance* instance);
    void initializeEarlyStrings();
    void initializeLazyStrings();
    std::vector<escargot::InternalAtomicString>& apis() { return m_apis; }

    typedef std::unordered_map<escargot::ESString*, escargot::InternalAtomicString,
            std::hash<escargot::ESString*>, std::equal_to<escargot::ESString*>,
            std::allocator<std::pair<const escargot::ESString*, escargot::InternalAtomicString> > > EntryPointsMap;

    EntryPointsMap& entryPoints() { return m_entryPoints; }

#define DECLARE_TIZEN_STRING(name) escargot::InternalAtomicString name;
    FOR_EACH_EARLY_TIZEN_STRINGS(DECLARE_TIZEN_STRING);
    FOR_EACH_LAZY_TIZEN_STRINGS(DECLARE_TIZEN_STRING);
#undef DECLARE_TIZEN_STRING

private:
    escargot::ESVMInstance* m_instance;
    std::vector<escargot::InternalAtomicString> m_apis;
    EntryPointsMap m_entryPoints;
    bool m_initialized;
};

/*
 * Extension: (tizen, utils, common, messageport, sensorservice...) * 1
 * ExtensionManager: (manager) * 1
 * ExtensionInstance: (tizen, utils, common, messageport, sensorservice...) * number of ESVMInstances
 * ExtensionManagerInstance: (manager) * number of ESVMInstances
 */

class ExtensionManagerInstance {
public:
    ExtensionManagerInstance(escargot::ESVMInstance* instance);
    ~ExtensionManagerInstance();
    static ExtensionManagerInstance* get(escargot::ESVMInstance* instance);
    static ExtensionManagerInstance* currentInstance();
    TizenStrings* strings() { return m_strings; }
    wrt::xwalk::ExtensionInstance* getExtensionInstanceFromCallingContext(escargot::ESVMInstance*);

private:
    struct ChunkData {
        ChunkData() { }
        ChunkData(uint8_t* buffer, size_t length)
            : m_buffer(buffer), m_length(length) { }
        uint8_t* m_buffer;
        size_t m_length;
    };

    typedef std::map<size_t, ChunkData> ChunkDataMap;
    typedef std::map<escargot::ESPointer*, wrt::xwalk::ExtensionInstance*> ExtensionInstanceMap;
    typedef std::vector<ESPostListener*> ESPostListenerVector;

    escargot::ESObject* initializeExtensionInstance(const char*);
    escargot::ESObject* createExtensionObject();
    size_t addChunk(uint8_t* buffer, size_t length);
    ChunkData getChunk(size_t chunkID);

    escargot::ESVMInstance* m_instance;
    ExtensionInstanceMap m_extensionInstances;
    ESPostListenerVector m_postListeners;
    ChunkDataMap m_chunkDataMap;
    size_t m_chunkID;
    TizenStrings* m_strings;

    // static members
    typedef std::map<escargot::ESVMInstance*, ExtensionManagerInstance*> ExtensionManagerInstanceMap;
    static wrt::xwalk::Extension* getExtension(const char* apiName);
    static ExtensionManagerInstanceMap s_extensionManagerInstances;
};

void initialize(escargot::ESVMInstance* instance);
void close(escargot::ESVMInstance* instance);

}

#endif // TIZEN_DEVICE_API

#endif // __TizenDeviceAPILoaderForEscargot__
