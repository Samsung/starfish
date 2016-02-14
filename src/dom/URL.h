#ifndef __StarFishURLObject__
#define __StarFishURLObject__

#include "dom/binding/ScriptWrappable.h"
// #include <uuid/uuid.h>

//"blob:http%3A//ohgyun.com/a9f2fd30-adf0-43dd-a413-9ee8423bec6e"

namespace StarFish {

class URL : public ScriptWrappable{
public:

    static String* makeUUID(){

        String* result;

        // uuid_t uuid;
        // uuid_generate_time_safe(uuid);
        // char uuid_str[37];
        // uuid_unparse_lower(uuid, uuid_str);
        // result = String::fromUTF8(uuid_str);

        // FIXME: mh.byun Fake Path(Temp Soluation!!)
        result = String::fromUTF8("a9f2fd30-adf0-43dd-a413-9ee8423b");
        srand(time(NULL));
        std::string tmp = std::to_string(rand()%10);
        result = result->concat(String::fromUTF8(tmp.c_str()));

        tmp = std::to_string(rand()%10);
        result = result->concat(String::fromUTF8(tmp.c_str()));

        tmp = std::to_string(rand()%10);
        result = result->concat(String::fromUTF8(tmp.c_str()));

        tmp = std::to_string(rand()%10);
        result = result->concat(String::fromUTF8(tmp.c_str()));

        return result;
    }

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

        String* objectURL = String::fromUTF8("blob:non-origin/");
        objectURL = objectURL->concat(makeUUID());
        Blob* blob = (Blob*)RawObject.asESPointer()->asESObject();
        m_starfish->fetchImage(objectURL,blob->size(),blob->data());
        return objectURL;

    }
protected:
    StarFish* m_starfish;
};


}

#endif
