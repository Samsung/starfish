/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarFishConfig.h"
#include "MimeType.h"

namespace StarFish {

MimeType::MimeType()
    : m_type(String::emptyString)
    , m_subtype(String::emptyString)
    , m_parameter(String::emptyString)

{
}

bool MimeType::isValid()
{
    return ((type()->length() > 0) && (subtype()->length() > 0));
}

bool MimeType::hasParameter()
{
    return (parameter()->length() > 0);
}

void MimeType::clear()
{
    m_type = String::emptyString;
    m_subtype = String::emptyString;
    m_parameter = String::emptyString;
}

String* MimeType::string()
{
    if (isValid()) {
        String* result =
            m_type->concat(String::createASCIIString("/"))->concat(m_subtype);
        if (hasParameter()) {
            return result->concat(String::createASCIIString(";"))
                ->concat(parameter());
        } else {
            return result;
        }
    }
    return String::emptyString;
}

MimeType MimeType::parseFromString(String* str)
{
    MimeType invalid, result;

    // Parsing a MIME type
    // https://mimesniff.spec.whatwg.org/#parse-a-mime-type
    if (str->length() < 1) {
        return invalid;
    }

    String* seq1 = str->toLower()->trim();
    size_t size1 = seq1->length();
    size_t s1 = seq1->indexOf('/');

    // Check "type" part
    if (size1 < 1 || s1 == SIZE_MAX || !(s1 > 0 && s1 < size1 - 1)) {
        return invalid;
    }
    for (size_t p = 0; p < s1; p++) {
        if ((int)(seq1->charAt(p)) > 127) {
            return invalid;
        }
    }
    result.setType(seq1->substring(0, s1));

    // Check "subType" part
    // TODO
    String* seq2 = seq1->substring(s1 + 1, size1 - s1 - 1);
    String* seq3 = String::emptyString;
    size_t size2 = seq2->length();
    size_t s2 = seq2->indexOf(';');
    if (s2 == SIZE_MAX) {
        s2 = size2;
    } else {
        seq3 = seq2->substring(s2 + 1, size2 - s2 - 1);
    }
    for (size_t p = 0; p < s2; p++) {
        if ((int)(seq2->charAt(p)) > 127) {
            return invalid;
        }
        if (String::isSpaceOrNewline(seq2->charAt(p))) {
            s2 = p;
            break;
        }
    }
    result.setSubtype(seq2->substring(0, s2));

    // Check "parameter" part
    // TODO
    size_t size3 = seq3->length();
    for (size_t p = 0; p < size3; p++) {
        if ((int)(seq3->charAt(p)) > 127) {
            return invalid;
        }
    }
    result.setParameter(seq3);
    return result;
}
}
