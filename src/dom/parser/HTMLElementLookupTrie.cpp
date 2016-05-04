#include "StarFishConfig.h"
#include "HTMLElementLookupTrie.h"

namespace StarFish {

QualifiedName lookupHTMLTag(StaticStrings& s, const char32_t* data, unsigned length)
{
    STARFISH_ASSERT(data);
    STARFISH_ASSERT(length);
    switch (length) {
    case 1:
        switch (data[0]) {
        case 'a':
            return s.m_aLocalName;
        case 'b':
            return s.m_bLocalName;
        case 'i':
            return s.m_iLocalName;
        case 'q':
            return s.m_qLocalName;
        case 'p':
            return s.m_pLocalName;
        case 's':
            return s.m_sLocalName;
        case 'u':
            return s.m_uLocalName;
        }
        return QualifiedName::emptyQualifiedName();
    case 2:
        switch (data[0]) {
        case 'b':
            if (data[1] == 'r')
                return s.m_brLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'e':
            if (data[1] == 'm')
                return s.m_emLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'd':
            switch (data[1]) {
            case 't':
                return s.m_dtLocalName;
            case 'd':
                return s.m_ddLocalName;
            case 'l':
                return s.m_dlLocalName;
            }
            return QualifiedName::emptyQualifiedName();
        case 'h':
            switch (data[1]) {
            case '1':
                return s.m_h1LocalName;
            case '3':
                return s.m_h3LocalName;
            case '2':
                return s.m_h2LocalName;
            case '5':
                return s.m_h5LocalName;
            case '4':
                return s.m_h4LocalName;
            case '6':
                return s.m_h6LocalName;
            case 'r':
                return s.m_hrLocalName;
            }
            return QualifiedName::emptyQualifiedName();
        case 'l':
            if (data[1] == 'i')
                return s.m_liLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'o':
            if (data[1] == 'l')
                return s.m_olLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'r':
            switch (data[1]) {
            case 'p':
                return s.m_rpLocalName;
            case 't':
                return s.m_rtLocalName;
            }
            return QualifiedName::emptyQualifiedName();
        case 'u':
            if (data[1] == 'l')
                return s.m_ulLocalName;
            return QualifiedName::emptyQualifiedName();
        case 't':
            switch (data[1]) {
            case 'h':
                return s.m_thLocalName;
            case 'r':
                return s.m_trLocalName;
            case 'd':
                return s.m_tdLocalName;
            case 't':
                return s.m_ttLocalName;
            }
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 3:
        switch (data[0]) {
        case 'c':
            if (data[1] == 'o' && data[2] == 'l')
                return s.m_colLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'b':
            switch (data[1]) {
            case 'i':
                if (data[2] == 'g')
                    return s.m_bigLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'd':
                switch (data[2]) {
                case 'i':
                    return s.m_bdiLocalName;
                case 'o':
                    return s.m_bdoLocalName;
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'd':
            switch (data[1]) {
            case 'i':
                switch (data[2]) {
                case 'r':
                    return s.m_dirLocalName;
                case 'v':
                    return s.m_divLocalName;
                }
                return QualifiedName::emptyQualifiedName();
            case 'e':
                if (data[2] == 'l')
                    return s.m_delLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'f':
                if (data[2] == 'n')
                    return s.m_dfnLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'i':
            switch (data[1]) {
            case 'm':
                if (data[2] == 'g')
                    return s.m_imgLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'n':
                if (data[2] == 's')
                    return s.m_insLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'k':
            if (data[1] == 'b' && data[2] == 'd')
                return s.m_kbdLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'm':
            if (data[1] == 'a' && data[2] == 'p')
                return s.m_mapLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'n':
            if (data[1] == 'a' && data[2] == 'v')
                return s.m_navLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'p':
            if (data[1] == 'r' && data[2] == 'e')
                return s.m_preLocalName;
            return QualifiedName::emptyQualifiedName();
        case 's':
            switch (data[1]) {
            case 'u':
                switch (data[2]) {
                case 'p':
                    return s.m_supLocalName;
                case 'b':
                    return s.m_subLocalName;
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'w':
            if (data[1] == 'b' && data[2] == 'r')
                return s.m_wbrLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'v':
            if (data[1] == 'a' && data[2] == 'r')
                return s.m_varLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'x':
            if (data[1] == 'm' && data[2] == 'p')
                return s.m_xmpLocalName;
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 4:
        switch (data[0]) {
        case 'a':
            switch (data[1]) {
            case 'r':
                if (data[2] == 'e' && data[3] == 'a')
                    return s.m_areaLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'b':
                if (data[2] == 'b' && data[3] == 'r')
                    return s.m_abbrLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'c':
            switch (data[1]) {
            case 'i':
                if (data[2] == 't' && data[3] == 'e')
                    return s.m_citeLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'o':
                if (data[2] == 'd' && data[3] == 'e')
                    return s.m_codeLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'b':
            switch (data[1]) {
            case 'a':
                if (data[2] == 's' && data[3] == 'e')
                    return s.m_baseLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'o':
                if (data[2] == 'd' && data[3] == 'y')
                    return s.m_bodyLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'f':
            switch (data[1]) {
            case 'o':
                switch (data[2]) {
                case 'r':
                    if (data[3] == 'm')
                        return s.m_formLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'n':
                    if (data[3] == 't')
                        return s.m_fontLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'h':
            switch (data[1]) {
            case 'e':
                if (data[2] == 'a' && data[3] == 'd')
                    return s.m_headLocalName;
                return QualifiedName::emptyQualifiedName();
            case 't':
                if (data[2] == 'm' && data[3] == 'l')
                    return s.m_htmlLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'm':
            switch (data[1]) {
            case 'a':
                switch (data[2]) {
                case 'i':
                    if (data[3] == 'n')
                        return s.m_mainLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'r':
                    if (data[3] == 'k')
                        return s.m_markLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            case 'e':
                switch (data[2]) {
                case 't':
                    if (data[3] == 'a')
                        return s.m_metaLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'n':
                    if (data[3] == 'u')
                        return s.m_menuLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'l':
            if (data[1] == 'i' && data[2] == 'n' && data[3] == 'k')
                return s.m_linkLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'n':
            if (data[1] == 'o' && data[2] == 'b' && data[3] == 'r')
                return s.m_nobrLocalName;
            return QualifiedName::emptyQualifiedName();
        case 's':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'm' && data[3] == 'p')
                    return s.m_sampLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'p':
                if (data[2] == 'a' && data[3] == 'n')
                    return s.m_spanLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'r':
            if (data[1] == 'u' && data[2] == 'b' && data[3] == 'y')
                return s.m_rubyLocalName;
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 5:
        switch (data[0]) {
        case 'a':
            switch (data[1]) {
            case 's':
                if (data[2] == 'i' && data[3] == 'd' && data[4] == 'e')
                    return s.m_asideLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'u':
                if (data[2] == 'd' && data[3] == 'i' && data[4] == 'o')
                    return s.m_audioLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'e':
            if (data[1] == 'm' && data[2] == 'b' && data[3] == 'e' && data[4] == 'd')
                return s.m_embedLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'f':
            if (data[1] == 'r' && data[2] == 'a' && data[3] == 'm' && data[4] == 'e')
                return s.m_frameLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'i':
            switch (data[1]) {
            case 'm':
                if (data[2] == 'a' && data[3] == 'g' && data[4] == 'e')
                    return s.m_imageLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'n':
                if (data[2] == 'p' && data[3] == 'u' && data[4] == 't')
                    return s.m_inputLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'm':
            if (data[1] == 'e' && data[2] == 't' && data[3] == 'e' && data[4] == 'r')
                return s.m_meterLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'l':
            switch (data[1]) {
            case 'a':
                switch (data[2]) {
                case 'y':
                    if (data[3] == 'e' && data[4] == 'r')
                        return s.m_layerLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'b':
                    if (data[3] == 'e' && data[4] == 'l')
                        return s.m_labelLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'p':
            if (data[1] == 'a' && data[2] == 'r' && data[3] == 'a' && data[4] == 'm')
                return s.m_paramLocalName;
            return QualifiedName::emptyQualifiedName();
        case 's':
            switch (data[1]) {
            case 'm':
                if (data[2] == 'a' && data[3] == 'l' && data[4] == 'l')
                    return s.m_smallLocalName;
                return QualifiedName::emptyQualifiedName();
            case 't':
                if (data[2] == 'y' && data[3] == 'l' && data[4] == 'e')
                    return s.m_styleLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 't':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'b' && data[3] == 'l' && data[4] == 'e')
                    return s.m_tableLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'b':
                if (data[2] == 'o' && data[3] == 'd' && data[4] == 'y')
                    return s.m_tbodyLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'f':
                if (data[2] == 'o' && data[3] == 'o' && data[4] == 't')
                    return s.m_tfootLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'i':
                if (data[2] == 't' && data[3] == 'l' && data[4] == 'e')
                    return s.m_titleLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'h':
                if (data[2] == 'e' && data[3] == 'a' && data[4] == 'd')
                    return s.m_theadLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'r':
                if (data[2] == 'a' && data[3] == 'c' && data[4] == 'k')
                    return s.m_trackLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'v':
            if (data[1] == 'i' && data[2] == 'd' && data[3] == 'e' && data[4] == 'o')
                return s.m_videoLocalName;
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 6:
        switch (data[0]) {
        case 'a':
            if (data[1] == 'p' && data[2] == 'p' && data[3] == 'l' && data[4] == 'e' && data[5] == 't')
                return s.m_appletLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'c':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'n' && data[3] == 'v' && data[4] == 'a' && data[5] == 's')
                    return s.m_canvasLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'e':
                if (data[2] == 'n' && data[3] == 't' && data[4] == 'e' && data[5] == 'r')
                    return s.m_centerLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'b':
            if (data[1] == 'u' && data[2] == 't' && data[3] == 't' && data[4] == 'o' && data[5] == 'n')
                return s.m_buttonLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'd':
            if (data[1] == 'i' && data[2] == 'a' && data[3] == 'l' && data[4] == 'o' && data[5] == 'g')
                return s.m_dialogLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'f':
            switch (data[1]) {
            case 'i':
                if (data[2] == 'g' && data[3] == 'u' && data[4] == 'r' && data[5] == 'e')
                    return s.m_figureLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'o':
                if (data[2] == 'o' && data[3] == 't' && data[4] == 'e' && data[5] == 'r')
                    return s.m_footerLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'i':
            if (data[1] == 'f' && data[2] == 'r' && data[3] == 'a' && data[4] == 'm' && data[5] == 'e')
                return s.m_iframeLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'h':
            switch (data[1]) {
            case 'e':
                if (data[2] == 'a' && data[3] == 'd' && data[4] == 'e' && data[5] == 'r')
                    return s.m_headerLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'g':
                if (data[2] == 'r' && data[3] == 'o' && data[4] == 'u' && data[5] == 'p')
                    return s.m_hgroupLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'k':
            if (data[1] == 'e' && data[2] == 'y' && data[3] == 'g' && data[4] == 'e' && data[5] == 'n')
                return s.m_keygenLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'l':
            if (data[1] == 'e' && data[2] == 'g' && data[3] == 'e' && data[4] == 'n' && data[5] == 'd')
                return s.m_legendLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'o':
            switch (data[1]) {
            case 'p':
                if (data[2] == 't' && data[3] == 'i' && data[4] == 'o' && data[5] == 'n')
                    return s.m_optionLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'b':
                if (data[2] == 'j' && data[3] == 'e' && data[4] == 'c' && data[5] == 't')
                    return s.m_objectLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'u':
                if (data[2] == 't' && data[3] == 'p' && data[4] == 'u' && data[5] == 't')
                    return s.m_outputLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 's':
            switch (data[1]) {
            case 'h':
                if (data[2] == 'a' && data[3] == 'd' && data[4] == 'o' && data[5] == 'w')
                    return s.m_shadowLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'c':
                if (data[2] == 'r' && data[3] == 'i' && data[4] == 'p' && data[5] == 't')
                    return s.m_scriptLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'e':
                if (data[2] == 'l' && data[3] == 'e' && data[4] == 'c' && data[5] == 't')
                    return s.m_selectLocalName;
                return QualifiedName::emptyQualifiedName();
            case 't':
                switch (data[2]) {
                case 'r':
                    switch (data[3]) {
                    case 'i':
                        if (data[4] == 'k' && data[5] == 'e')
                            return s.m_strikeLocalName;
                        return QualifiedName::emptyQualifiedName();
                    case 'o':
                        if (data[4] == 'n' && data[5] == 'g')
                            return s.m_strongLocalName;
                        return QualifiedName::emptyQualifiedName();
                    }
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            case 'o':
                if (data[2] == 'u' && data[3] == 'r' && data[4] == 'c' && data[5] == 'e')
                    return s.m_sourceLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 7:
        switch (data[0]) {
        case 'a':
            switch (data[1]) {
            case 'c':
                if (data[2] == 'r' && data[3] == 'o' && data[4] == 'n' && data[5] == 'y' && data[6] == 'm')
                    return s.m_acronymLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'r':
                if (data[2] == 't' && data[3] == 'i' && data[4] == 'c' && data[5] == 'l' && data[6] == 'e')
                    return s.m_articleLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'd':
                if (data[2] == 'd' && data[3] == 'r' && data[4] == 'e' && data[5] == 's' && data[6] == 's')
                    return s.m_addressLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'c':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'p' && data[3] == 't' && data[4] == 'i' && data[5] == 'o' && data[6] == 'n')
                    return s.m_captionLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'o':
                switch (data[2]) {
                case 'm':
                    if (data[3] == 'm' && data[4] == 'a' && data[5] == 'n' && data[6] == 'd')
                        return s.m_commandLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'n':
                    if (data[3] == 't' && data[4] == 'e' && data[5] == 'n' && data[6] == 't')
                        return s.m_contentLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'b':
            if (data[1] == 'g' && data[2] == 's' && data[3] == 'o' && data[4] == 'u' && data[5] == 'n' && data[6] == 'd')
                return s.m_bgsoundLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'd':
            if (data[1] == 'e' && data[2] == 't' && data[3] == 'a' && data[4] == 'i' && data[5] == 'l' && data[6] == 's')
                return s.m_detailsLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'm':
            if (data[1] == 'a' && data[2] == 'r' && data[3] == 'q' && data[4] == 'u' && data[5] == 'e' && data[6] == 'e')
                return s.m_marqueeLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'l':
            if (data[1] == 'i' && data[2] == 's' && data[3] == 't' && data[4] == 'i' && data[5] == 'n' && data[6] == 'g')
                return s.m_listingLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'n':
            switch (data[1]) {
            case 'o':
                switch (data[2]) {
                case 'e':
                    if (data[3] == 'm' && data[4] == 'b' && data[5] == 'e' && data[6] == 'd')
                        return s.m_noembedLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'l':
                    if (data[3] == 'a' && data[4] == 'y' && data[5] == 'e' && data[6] == 'r')
                        return s.m_nolayerLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 's':
            switch (data[1]) {
            case 'u':
                if (data[2] == 'm' && data[3] == 'm' && data[4] == 'a' && data[5] == 'r' && data[6] == 'y')
                    return s.m_summaryLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'e':
                if (data[2] == 'c' && data[3] == 't' && data[4] == 'i' && data[5] == 'o' && data[6] == 'n')
                    return s.m_sectionLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 8:
        switch (data[0]) {
        case 'c':
            if (data[1] == 'o' && data[2] == 'l' && data[3] == 'g' && data[4] == 'r' && data[5] == 'o' && data[6] == 'u' && data[7] == 'p')
                return s.m_colgroupLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'b':
            if (data[1] == 'a' && data[2] == 's' && data[3] == 'e' && data[4] == 'f' && data[5] == 'o' && data[6] == 'n' && data[7] == 't')
                return s.m_basefontLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'd':
            if (data[1] == 'a' && data[2] == 't' && data[3] == 'a' && data[4] == 'l' && data[5] == 'i' && data[6] == 's' && data[7] == 't')
                return s.m_datalistLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'f':
            switch (data[1]) {
            case 'i':
                if (data[2] == 'e' && data[3] == 'l' && data[4] == 'd' && data[5] == 's' && data[6] == 'e' && data[7] == 't')
                    return s.m_fieldsetLocalName;
                return QualifiedName::emptyQualifiedName();
            case 'r':
                if (data[2] == 'a' && data[3] == 'm' && data[4] == 'e' && data[5] == 's' && data[6] == 'e' && data[7] == 't')
                    return s.m_framesetLocalName;
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'o':
            if (data[1] == 'p' && data[2] == 't' && data[3] == 'g' && data[4] == 'r' && data[5] == 'o' && data[6] == 'u' && data[7] == 'p')
                return s.m_optgroupLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'n':
            switch (data[1]) {
            case 'o':
                switch (data[2]) {
                case 's':
                    if (data[3] == 'c' && data[4] == 'r' && data[5] == 'i' && data[6] == 'p' && data[7] == 't')
                        return s.m_noscriptLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'f':
                    if (data[3] == 'r' && data[4] == 'a' && data[5] == 'm' && data[6] == 'e' && data[7] == 's')
                        return s.m_noframesLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        case 'p':
            if (data[1] == 'r' && data[2] == 'o' && data[3] == 'g' && data[4] == 'r' && data[5] == 'e' && data[6] == 's' && data[7] == 's')
                return s.m_progressLocalName;
            return QualifiedName::emptyQualifiedName();
        case 't':
            switch (data[1]) {
            case 'e':
                switch (data[2]) {
                case 'x':
                    if (data[3] == 't' && data[4] == 'a' && data[5] == 'r' && data[6] == 'e' && data[7] == 'a')
                        return s.m_textareaLocalName;
                    return QualifiedName::emptyQualifiedName();
                case 'm':
                    if (data[3] == 'p' && data[4] == 'l' && data[5] == 'a' && data[6] == 't' && data[7] == 'e')
                        return s.m_templateLocalName;
                    return QualifiedName::emptyQualifiedName();
                }
                return QualifiedName::emptyQualifiedName();
            }
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 9:
        switch (data[0]) {
        case 'p':
            if (data[1] == 'l' && data[2] == 'a' && data[3] == 'i' && data[4] == 'n' && data[5] == 't' && data[6] == 'e' && data[7] == 'x' && data[8] == 't')
                return s.m_plaintextLocalName;
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();
    case 10:
        switch (data[0]) {
        case 'b':
            if (data[1] == 'l' && data[2] == 'o' && data[3] == 'c' && data[4] == 'k' && data[5] == 'q' && data[6] == 'u' && data[7] == 'o' && data[8] == 't' && data[9] == 'e')
                return s.m_blockquoteLocalName;
            return QualifiedName::emptyQualifiedName();
        case 'f':
            if (data[1] == 'i' && data[2] == 'g' && data[3] == 'c' && data[4] == 'a' && data[5] == 'p' && data[6] == 't' && data[7] == 'i' && data[8] == 'o' && data[9] == 'n')
                return s.m_figcaptionLocalName;
            return QualifiedName::emptyQualifiedName();
        }
        return QualifiedName::emptyQualifiedName();

    }
    return QualifiedName::emptyQualifiedName();
}

}
