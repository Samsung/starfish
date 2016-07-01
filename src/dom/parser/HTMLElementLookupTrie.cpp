/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
            return s.m_aTagName;
        case 'b':
            return s.m_bTagName;
        case 'i':
            return s.m_iTagName;
        case 'q':
            return s.m_qTagName;
        case 'p':
            return s.m_pTagName;
        case 's':
            return s.m_sTagName;
        case 'u':
            return s.m_uTagName;
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 2:
        switch (data[0]) {
        case 'b':
            if (data[1] == 'r')
                return s.m_brTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'e':
            if (data[1] == 'm')
                return s.m_emTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'd':
            switch (data[1]) {
            case 't':
                return s.m_dtTagName;
            case 'd':
                return s.m_ddTagName;
            case 'l':
                return s.m_dlTagName;
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'h':
            switch (data[1]) {
            case '1':
                return s.m_h1TagName;
            case '3':
                return s.m_h3TagName;
            case '2':
                return s.m_h2TagName;
            case '5':
                return s.m_h5TagName;
            case '4':
                return s.m_h4TagName;
            case '6':
                return s.m_h6TagName;
            case 'r':
                return s.m_hrTagName;
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'l':
            if (data[1] == 'i')
                return s.m_liTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'o':
            if (data[1] == 'l')
                return s.m_olTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'r':
            switch (data[1]) {
            case 'p':
                return s.m_rpTagName;
            case 't':
                return s.m_rtTagName;
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'u':
            if (data[1] == 'l')
                return s.m_ulTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 't':
            switch (data[1]) {
            case 'h':
                return s.m_thTagName;
            case 'r':
                return s.m_trTagName;
            case 'd':
                return s.m_tdTagName;
            case 't':
                return s.m_ttTagName;
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 3:
        switch (data[0]) {
        case 'c':
            if (data[1] == 'o' && data[2] == 'l')
                return s.m_colTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'b':
            switch (data[1]) {
            case 'i':
                if (data[2] == 'g')
                    return s.m_bigTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'd':
                switch (data[2]) {
                case 'i':
                    return s.m_bdiTagName;
                case 'o':
                    return s.m_bdoTagName;
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'd':
            switch (data[1]) {
            case 'i':
                switch (data[2]) {
                case 'r':
                    return s.m_dirTagName;
                case 'v':
                    return s.m_divTagName;
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'e':
                if (data[2] == 'l')
                    return s.m_delTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'f':
                if (data[2] == 'n')
                    return s.m_dfnTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'i':
            switch (data[1]) {
            case 'm':
                if (data[2] == 'g')
                    return s.m_imgTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'n':
                if (data[2] == 's')
                    return s.m_insTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'k':
            if (data[1] == 'b' && data[2] == 'd')
                return s.m_kbdTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'm':
            if (data[1] == 'a' && data[2] == 'p')
                return s.m_mapTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'n':
            if (data[1] == 'a' && data[2] == 'v')
                return s.m_navTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'p':
            if (data[1] == 'r' && data[2] == 'e')
                return s.m_preTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 's':
            switch (data[1]) {
            case 'u':
                switch (data[2]) {
                case 'p':
                    return s.m_supTagName;
                case 'b':
                    return s.m_subTagName;
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'w':
            if (data[1] == 'b' && data[2] == 'r')
                return s.m_wbrTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'v':
            if (data[1] == 'a' && data[2] == 'r')
                return s.m_varTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'x':
            if (data[1] == 'm' && data[2] == 'p')
                return s.m_xmpTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 4:
        switch (data[0]) {
        case 'a':
            switch (data[1]) {
            case 'r':
                if (data[2] == 'e' && data[3] == 'a')
                    return s.m_areaTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'b':
                if (data[2] == 'b' && data[3] == 'r')
                    return s.m_abbrTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'c':
            switch (data[1]) {
            case 'i':
                if (data[2] == 't' && data[3] == 'e')
                    return s.m_citeTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'o':
                if (data[2] == 'd' && data[3] == 'e')
                    return s.m_codeTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'b':
            switch (data[1]) {
            case 'a':
                if (data[2] == 's' && data[3] == 'e')
                    return s.m_baseTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'o':
                if (data[2] == 'd' && data[3] == 'y')
                    return s.m_bodyTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'f':
            switch (data[1]) {
            case 'o':
                switch (data[2]) {
                case 'r':
                    if (data[3] == 'm')
                        return s.m_formTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'n':
                    if (data[3] == 't')
                        return s.m_fontTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'h':
            switch (data[1]) {
            case 'e':
                if (data[2] == 'a' && data[3] == 'd')
                    return s.m_headTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 't':
                if (data[2] == 'm' && data[3] == 'l')
                    return s.m_htmlTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'm':
            switch (data[1]) {
            case 'a':
                switch (data[2]) {
                case 'i':
                    if (data[3] == 'n')
                        return s.m_mainTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'r':
                    if (data[3] == 'k')
                        return s.m_markTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'e':
                switch (data[2]) {
                case 't':
                    if (data[3] == 'a')
                        return s.m_metaTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'n':
                    if (data[3] == 'u')
                        return s.m_menuTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'l':
            if (data[1] == 'i' && data[2] == 'n' && data[3] == 'k')
                return s.m_linkTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'n':
            if (data[1] == 'o' && data[2] == 'b' && data[3] == 'r')
                return s.m_nobrTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 's':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'm' && data[3] == 'p')
                    return s.m_sampTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'p':
                if (data[2] == 'a' && data[3] == 'n')
                    return s.m_spanTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'r':
            if (data[1] == 'u' && data[2] == 'b' && data[3] == 'y')
                return s.m_rubyTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 5:
        switch (data[0]) {
        case 'a':
            switch (data[1]) {
            case 's':
                if (data[2] == 'i' && data[3] == 'd' && data[4] == 'e')
                    return s.m_asideTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'u':
                if (data[2] == 'd' && data[3] == 'i' && data[4] == 'o')
                    return s.m_audioTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'e':
            if (data[1] == 'm' && data[2] == 'b' && data[3] == 'e' && data[4] == 'd')
                return s.m_embedTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'f':
            if (data[1] == 'r' && data[2] == 'a' && data[3] == 'm' && data[4] == 'e')
                return s.m_frameTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'i':
            switch (data[1]) {
            case 'm':
                if (data[2] == 'a' && data[3] == 'g' && data[4] == 'e')
                    return s.m_imageTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'n':
                if (data[2] == 'p' && data[3] == 'u' && data[4] == 't')
                    return s.m_inputTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'm':
            if (data[1] == 'e' && data[2] == 't' && data[3] == 'e' && data[4] == 'r')
                return s.m_meterTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'l':
            switch (data[1]) {
            case 'a':
                switch (data[2]) {
                case 'y':
                    if (data[3] == 'e' && data[4] == 'r')
                        return s.m_layerTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'b':
                    if (data[3] == 'e' && data[4] == 'l')
                        return s.m_labelTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'p':
            if (data[1] == 'a' && data[2] == 'r' && data[3] == 'a' && data[4] == 'm')
                return s.m_paramTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 's':
            switch (data[1]) {
            case 'm':
                if (data[2] == 'a' && data[3] == 'l' && data[4] == 'l')
                    return s.m_smallTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 't':
                if (data[2] == 'y' && data[3] == 'l' && data[4] == 'e')
                    return s.m_styleTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 't':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'b' && data[3] == 'l' && data[4] == 'e')
                    return s.m_tableTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'b':
                if (data[2] == 'o' && data[3] == 'd' && data[4] == 'y')
                    return s.m_tbodyTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'f':
                if (data[2] == 'o' && data[3] == 'o' && data[4] == 't')
                    return s.m_tfootTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'i':
                if (data[2] == 't' && data[3] == 'l' && data[4] == 'e')
                    return s.m_titleTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'h':
                if (data[2] == 'e' && data[3] == 'a' && data[4] == 'd')
                    return s.m_theadTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'r':
                if (data[2] == 'a' && data[3] == 'c' && data[4] == 'k')
                    return s.m_trackTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'v':
            if (data[1] == 'i' && data[2] == 'd' && data[3] == 'e' && data[4] == 'o')
                return s.m_videoTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 6:
        switch (data[0]) {
        case 'a':
            if (data[1] == 'p' && data[2] == 'p' && data[3] == 'l' && data[4] == 'e' && data[5] == 't')
                return s.m_appletTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'c':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'n' && data[3] == 'v' && data[4] == 'a' && data[5] == 's')
                    return s.m_canvasTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'e':
                if (data[2] == 'n' && data[3] == 't' && data[4] == 'e' && data[5] == 'r')
                    return s.m_centerTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'b':
            if (data[1] == 'u' && data[2] == 't' && data[3] == 't' && data[4] == 'o' && data[5] == 'n')
                return s.m_buttonTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'd':
            if (data[1] == 'i' && data[2] == 'a' && data[3] == 'l' && data[4] == 'o' && data[5] == 'g')
                return s.m_dialogTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'f':
            switch (data[1]) {
            case 'i':
                if (data[2] == 'g' && data[3] == 'u' && data[4] == 'r' && data[5] == 'e')
                    return s.m_figureTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'o':
                if (data[2] == 'o' && data[3] == 't' && data[4] == 'e' && data[5] == 'r')
                    return s.m_footerTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'i':
            if (data[1] == 'f' && data[2] == 'r' && data[3] == 'a' && data[4] == 'm' && data[5] == 'e')
                return s.m_iframeTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'h':
            switch (data[1]) {
            case 'e':
                if (data[2] == 'a' && data[3] == 'd' && data[4] == 'e' && data[5] == 'r')
                    return s.m_headerTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'g':
                if (data[2] == 'r' && data[3] == 'o' && data[4] == 'u' && data[5] == 'p')
                    return s.m_hgroupTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'k':
            if (data[1] == 'e' && data[2] == 'y' && data[3] == 'g' && data[4] == 'e' && data[5] == 'n')
                return s.m_keygenTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'l':
            if (data[1] == 'e' && data[2] == 'g' && data[3] == 'e' && data[4] == 'n' && data[5] == 'd')
                return s.m_legendTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'o':
            switch (data[1]) {
            case 'p':
                if (data[2] == 't' && data[3] == 'i' && data[4] == 'o' && data[5] == 'n')
                    return s.m_optionTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'b':
                if (data[2] == 'j' && data[3] == 'e' && data[4] == 'c' && data[5] == 't')
                    return s.m_objectTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'u':
                if (data[2] == 't' && data[3] == 'p' && data[4] == 'u' && data[5] == 't')
                    return s.m_outputTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 's':
            switch (data[1]) {
            case 'h':
                if (data[2] == 'a' && data[3] == 'd' && data[4] == 'o' && data[5] == 'w')
                    return s.m_shadowTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'c':
                if (data[2] == 'r' && data[3] == 'i' && data[4] == 'p' && data[5] == 't')
                    return s.m_scriptTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'e':
                if (data[2] == 'l' && data[3] == 'e' && data[4] == 'c' && data[5] == 't')
                    return s.m_selectTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 't':
                switch (data[2]) {
                case 'r':
                    switch (data[3]) {
                    case 'i':
                        if (data[4] == 'k' && data[5] == 'e')
                            return s.m_strikeTagName;
                        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                    case 'o':
                        if (data[4] == 'n' && data[5] == 'g')
                            return s.m_strongTagName;
                        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                    }
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'o':
                if (data[2] == 'u' && data[3] == 'r' && data[4] == 'c' && data[5] == 'e')
                    return s.m_sourceTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 7:
        switch (data[0]) {
        case 'a':
            switch (data[1]) {
            case 'c':
                if (data[2] == 'r' && data[3] == 'o' && data[4] == 'n' && data[5] == 'y' && data[6] == 'm')
                    return s.m_acronymTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'r':
                if (data[2] == 't' && data[3] == 'i' && data[4] == 'c' && data[5] == 'l' && data[6] == 'e')
                    return s.m_articleTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'd':
                if (data[2] == 'd' && data[3] == 'r' && data[4] == 'e' && data[5] == 's' && data[6] == 's')
                    return s.m_addressTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'c':
            switch (data[1]) {
            case 'a':
                if (data[2] == 'p' && data[3] == 't' && data[4] == 'i' && data[5] == 'o' && data[6] == 'n')
                    return s.m_captionTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'o':
                switch (data[2]) {
                case 'm':
                    if (data[3] == 'm' && data[4] == 'a' && data[5] == 'n' && data[6] == 'd')
                        return s.m_commandTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'n':
                    if (data[3] == 't' && data[4] == 'e' && data[5] == 'n' && data[6] == 't')
                        return s.m_contentTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'b':
            if (data[1] == 'g' && data[2] == 's' && data[3] == 'o' && data[4] == 'u' && data[5] == 'n' && data[6] == 'd')
                return s.m_bgsoundTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'd':
            if (data[1] == 'e' && data[2] == 't' && data[3] == 'a' && data[4] == 'i' && data[5] == 'l' && data[6] == 's')
                return s.m_detailsTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'm':
            if (data[1] == 'a' && data[2] == 'r' && data[3] == 'q' && data[4] == 'u' && data[5] == 'e' && data[6] == 'e')
                return s.m_marqueeTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'l':
            if (data[1] == 'i' && data[2] == 's' && data[3] == 't' && data[4] == 'i' && data[5] == 'n' && data[6] == 'g')
                return s.m_listingTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'n':
            switch (data[1]) {
            case 'o':
                switch (data[2]) {
                case 'e':
                    if (data[3] == 'm' && data[4] == 'b' && data[5] == 'e' && data[6] == 'd')
                        return s.m_noembedTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'l':
                    if (data[3] == 'a' && data[4] == 'y' && data[5] == 'e' && data[6] == 'r')
                        return s.m_nolayerTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 's':
            switch (data[1]) {
            case 'u':
                if (data[2] == 'm' && data[3] == 'm' && data[4] == 'a' && data[5] == 'r' && data[6] == 'y')
                    return s.m_summaryTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'e':
                if (data[2] == 'c' && data[3] == 't' && data[4] == 'i' && data[5] == 'o' && data[6] == 'n')
                    return s.m_sectionTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 8:
        switch (data[0]) {
        case 'c':
            if (data[1] == 'o' && data[2] == 'l' && data[3] == 'g' && data[4] == 'r' && data[5] == 'o' && data[6] == 'u' && data[7] == 'p')
                return s.m_colgroupTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'b':
            if (data[1] == 'a' && data[2] == 's' && data[3] == 'e' && data[4] == 'f' && data[5] == 'o' && data[6] == 'n' && data[7] == 't')
                return s.m_basefontTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'd':
            if (data[1] == 'a' && data[2] == 't' && data[3] == 'a' && data[4] == 'l' && data[5] == 'i' && data[6] == 's' && data[7] == 't')
                return s.m_datalistTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'f':
            switch (data[1]) {
            case 'i':
                if (data[2] == 'e' && data[3] == 'l' && data[4] == 'd' && data[5] == 's' && data[6] == 'e' && data[7] == 't')
                    return s.m_fieldsetTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            case 'r':
                if (data[2] == 'a' && data[3] == 'm' && data[4] == 'e' && data[5] == 's' && data[6] == 'e' && data[7] == 't')
                    return s.m_framesetTagName;
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'o':
            if (data[1] == 'p' && data[2] == 't' && data[3] == 'g' && data[4] == 'r' && data[5] == 'o' && data[6] == 'u' && data[7] == 'p')
                return s.m_optgroupTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'n':
            switch (data[1]) {
            case 'o':
                switch (data[2]) {
                case 's':
                    if (data[3] == 'c' && data[4] == 'r' && data[5] == 'i' && data[6] == 'p' && data[7] == 't')
                        return s.m_noscriptTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'f':
                    if (data[3] == 'r' && data[4] == 'a' && data[5] == 'm' && data[6] == 'e' && data[7] == 's')
                        return s.m_noframesTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'p':
            if (data[1] == 'r' && data[2] == 'o' && data[3] == 'g' && data[4] == 'r' && data[5] == 'e' && data[6] == 's' && data[7] == 's')
                return s.m_progressTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 't':
            switch (data[1]) {
            case 'e':
                switch (data[2]) {
                case 'x':
                    if (data[3] == 't' && data[4] == 'a' && data[5] == 'r' && data[6] == 'e' && data[7] == 'a')
                        return s.m_textareaTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                case 'm':
                    if (data[3] == 'p' && data[4] == 'l' && data[5] == 'a' && data[6] == 't' && data[7] == 'e')
                        return s.m_templateTagName;
                    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
                }
                return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
            }
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 9:
        switch (data[0]) {
        case 'p':
            if (data[1] == 'l' && data[2] == 'a' && data[3] == 'i' && data[4] == 'n' && data[5] == 't' && data[6] == 'e' && data[7] == 'x' && data[8] == 't')
                return s.m_plaintextTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
    case 10:
        switch (data[0]) {
        case 'b':
            if (data[1] == 'l' && data[2] == 'o' && data[3] == 'c' && data[4] == 'k' && data[5] == 'q' && data[6] == 'u' && data[7] == 'o' && data[8] == 't' && data[9] == 'e')
                return s.m_blockquoteTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        case 'f':
            if (data[1] == 'i' && data[2] == 'g' && data[3] == 'c' && data[4] == 'a' && data[5] == 'p' && data[6] == 't' && data[7] == 'i' && data[8] == 'o' && data[9] == 'n')
                return s.m_figcaptionTagName;
            return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
        }
        return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());

    }
    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::emptyAtomicString());
}

}
