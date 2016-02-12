#ifndef __StarFishURLObject__
#define __StarFishURLObject__

#include "dom/binding/ScriptWrappable.h"
// #include <uuid/uuid.h>

//"blob:http%3A//ohgyun.com/a9f2fd30-adf0-43dd-a413-9ee8423bec6e"

namespace StarFish {

class URL : public ScriptWrappable{
public:

    URL(ScriptBindingInstance* instance,StarFish* starfish)
    {
        m_starfish = starfish;

        //TODO:mh.byun(Remove this?)
        initScriptWrappable(this,instance);

        escargot::ESFunctionObject* urlCreateObjectURLFunction = escargot::ESFunctionObject::create(NULL, [](escargot::ESVMInstance* instance) -> escargot::ESValue {
            escargot::ESValue v = instance->currentExecutionContext()->resolveThisBinding();
            if (v.isObject()) {
                URL* url = (URL*)v.asESPointer()->asESObject();
                String* result = url->createObjectURL(instance->currentExecutionContext()->readArgument(0));
                return escargot::ESValue(escargot::ESString::create(result->utf8Data()));
            }
            return escargot::ESValue(escargot::ESValue::ESNull);
        }, escargot::ESString::create("createObjectURL"), 1, false);
        defineDataProperty(escargot::ESString::create("createObjectURL"), false, false, false, urlCreateObjectURLFunction);
    }

    String* createObjectURL(escargot::ESValue RawObject){

        // uuid_t uuid;
        // uuid_generate_time_safe(uuid);
        // char uuid_str[37];
        // uuid_unparse_lower(uuid, uuid_str);

        String* objectURL = String::fromUTF8("blob:non-origin/");
        // objectURL = objectURL->concat(String::fromUTF8(uuid_str));

        // Blob* blob = (Blob*)RawObject.asESPointer()->asESObject();
        // m_starfish->fetchImage(objectURL,blob->size(),blob->data());
        return objectURL;
    }
protected:
    StarFish* m_starfish;
};


}

#endif
