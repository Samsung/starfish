#ifndef __StarFishURLObject__
#define __StarFishURLObject__

#include "dom/binding/ScriptWrappable.h"
// #include <uuid/uuid.h>

//"blob:http%3A//ohgyun.com/a9f2fd30-adf0-43dd-a413-9ee8423bec6e"

namespace StarFish {

class URL{
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

    static String* createObjectURL(Blob* blob,StarFish* starfish){

        String* objectURL = String::fromUTF8("blob:non-origin/");
        objectURL = objectURL->concat(makeUUID());
        starfish->fetchImage(objectURL,blob->size(),blob->data());
        return objectURL;

    }
};


}

#endif
