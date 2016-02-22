
#include "StarFishConfig.h"
#include "ComputedStyle.h"

#include "StarFish.h"

namespace StarFish {

void ComputedStyle::loadResources(StarFish* sf)
{
    float fontSize = m_inheritedStyles.m_fontSize.fixed();
    char style = 1;
    char fontWeight = 4;

    switch (m_inheritedStyles.m_fontWeight) {
    case OneHundredFontWeightValue:
        fontWeight = 1;
        break;
    case TwoHundredsFontWeightValue:
        fontWeight = 2;
        break;
    case ThreeHundredsFontWeightValue:
        fontWeight = 3;
        break;
    case NormalFontWeightValue:
    case FourHundredsFontWeightValue:
        fontWeight = 4;
        break;
    case FiveHundredsFontWeightValue:
        fontWeight = 5;
        break;
    case SixHundredsFontWeightValue:
        fontWeight = 6;
        break;
    case BoldFontWeightValue:
    case SevenHundredsFontWeightValue:
        fontWeight = 7;
        break;
    case EightHundredsFontWeightValue:
        fontWeight = 8;
        break;
    case NineHundredsFontWeightValue:
        fontWeight = 9;
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }


#ifdef STARFISH_ENABLE_PIXEL_TEST
    if (g_enablePixelTest)
        m_font = sf->fetchFont(String::fromUTF8("Ahem"), fontSize, style, fontWeight);
    else
        m_font = sf->fetchFont(String::fromUTF8("Ubuntu"), fontSize, style, fontWeight);
#else
    m_font = sf->fetchFont(String::emptyString, fontSize, style, fontWeight);
#endif

    if (!bgImage()->equals(String::emptyString)) {
        setBgImageData(sf->fetchImage(sf->makeResourcePath(bgImage())));
    }
}

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle, ComputedStyle* newStyle)
{
    if (memcmp(&oldStyle->m_inheritedStyles, &newStyle->m_inheritedStyles, sizeof (ComputedStyle::InheritedStyles)) != 0) {
        return ComputedStyleDamage::ComputedStyleDamageInherited;
    }
    return ComputedStyleDamage::ComputedStyleDamageNone;
}


}
