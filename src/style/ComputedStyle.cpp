
#include "StarFishConfig.h"
#include "ComputedStyle.h"

#include "StarFish.h"

namespace StarFish {

void ComputedStyle::loadResources(StarFish* sf)
{
    float fontSize = m_inheritedStyles.m_fontSize.fixed();

    char style = m_inheritedStyles.m_fontStyle;
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
    else {
        if (sf->startUpFlag() & StarFishStartUpFlag::enableRegressionTest)
            m_font = sf->fetchFont(String::fromUTF8("Arial"), fontSize, style, fontWeight);
        else
            m_font = sf->fetchFont(String::emptyString, fontSize, style, fontWeight);
    }
#else
    m_font = sf->fetchFont(String::emptyString, fontSize, style, fontWeight);
#endif

    if (!backgroundImage()->equals(String::emptyString)) {
        setBackgroundImageData(sf->fetchImage(sf->makeResourcePath(backgroundImage())));
    }
}

ComputedStyleDamage compareStyle(ComputedStyle* oldStyle, ComputedStyle* newStyle)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    if (memcmp(&oldStyle->m_inheritedStyles, &newStyle->m_inheritedStyles, sizeof(ComputedStyle::InheritedStyles)) != 0) {
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageInherited | damage);
    }

    if (newStyle->m_inheritedStyles.m_color != oldStyle->m_inheritedStyles.m_color)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamagePainting | damage);

    if (newStyle->m_inheritedStyles.m_direction != oldStyle->m_inheritedStyles.m_direction)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_inheritedStyles.m_fontSize != oldStyle->m_inheritedStyles.m_fontSize)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_inheritedStyles.m_fontWeight != oldStyle->m_inheritedStyles.m_fontWeight)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_inheritedStyles.m_letterSpacing != oldStyle->m_inheritedStyles.m_letterSpacing)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_inheritedStyles.m_lineHeight != oldStyle->m_inheritedStyles.m_lineHeight)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_inheritedStyles.m_textAlign != oldStyle->m_inheritedStyles.m_textAlign)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_inheritedStyles.m_visibility != oldStyle->m_inheritedStyles.m_visibility)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamagePainting | damage);

    if (newStyle->m_display != oldStyle->m_display)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    if (newStyle->m_originalDisplay != oldStyle->m_originalDisplay)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    if (newStyle->m_position != oldStyle->m_position)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    if (newStyle->m_textOverflow != oldStyle->m_textOverflow)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_width != oldStyle->m_width)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_height != oldStyle->m_height)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_verticalAlign != oldStyle->m_verticalAlign)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_verticalAlignLength != oldStyle->m_verticalAlignLength)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);

    if (newStyle->m_overflowX != oldStyle->m_overflowX)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    // if (newStyle->m_overflowY != oldStyle->m_overflowY)
    //     damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    // NOTE.
    // text-decoration is not inherited.
    // but it influence its child boxes, within it's inline formatting context
    // and is further propagated to any in-flow block-level boxes that split the inline (see section 9.2.1.1).
    // we store text-decoration values in FrameBlockBox
    // so we need to rebuild frame from this point
    // https://www.w3.org/TR/CSS2/text.html#propdef-text-decoration
    if (newStyle->m_textDecoration != oldStyle->m_textDecoration)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    if (newStyle->m_opacity != oldStyle->m_opacity)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    // FIXME changing z-index not always cause tree-rebuild
    if (newStyle->m_zIndex != oldStyle->m_zIndex)
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageRebuildFrame | damage);

    if (newStyle->m_background == NULL && oldStyle->m_background == NULL) {

    } else if (newStyle->m_background == NULL || oldStyle->m_background == NULL) {
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    } else if (*newStyle->m_background != *oldStyle->m_background) {
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamagePainting | damage);
    }

    if (newStyle->m_surround == NULL && oldStyle->m_surround == NULL) {

    } else if (newStyle->m_surround == NULL || oldStyle->m_surround == NULL) {
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    } else if (*newStyle->m_surround != *oldStyle->m_surround) {
        damage = (ComputedStyleDamage)(ComputedStyleDamage::ComputedStyleDamageLayout | damage);
    }


    return damage;
}
}
