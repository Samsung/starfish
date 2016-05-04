#include "StarFishConfig.h"
#include "Attribute.h"

namespace StarFish {

Attribute* findAttributeInVector(AttributeVector& attr, const QualifiedName& attributeName)
{
    for (size_t i = 0; i < attr.size(); i ++) {
        if (attr[i].name() == attributeName)
            return &attr[i];
    }
    return nullptr;
}

}
