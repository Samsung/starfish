/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "HTMLParser.h"
#include "AtomicHTMLToken.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLScriptElement.h"

namespace StarFish {

void HTMLParser::startParse()
{
    m_document->setInParsing(true);
}

void HTMLParser::endParse()
{
    m_treeBuilder.flush();
    m_treeBuilder.finished();
    m_treeBuilder.detach();
    m_document->setInParsing(false);
    if (!m_treeBuilder.isParsingFragment()) {
        m_document->setReadyState(DocumentReadyStateInteractive);
        m_document->endDocumentParsing();
        m_document->notifyDomContentLoaded();
    }
}

void HTMLParser::parseStep(bool shouldEndParseWhenThereIsNoToken)
{
    while (true) {
        if (m_treeBuilder.hasParserBlockingScript()) {
            TextPosition pos;
            HTMLScriptElement* script =
                m_treeBuilder.takeScriptToProcess(pos)->asHTMLScriptElement();
            script->clearParserInserted();
            bool forceSync = !shouldEndParseWhenThereIsNoToken;
            if (m_document->openFunctionExplicitCalled()) {
                forceSync = true;
            }
            bool shouldStop = script->executeScript(forceSync, true);
            script->markScriptExecuted();
            if (shouldStop) {
                break;
            }
        }

        if (!m_tokenizer.nextToken(m_input.current(), token())) {
            if (shouldEndParseWhenThereIsNoToken) {
                endParse();
            }
            break;
        }
        HTMLToken& rawToken = token();
        AtomicHTMLToken at(m_starFish, rawToken);

        // We clear the rawToken in case constructTreeFromAtomicToken
        // synchronously re-enters the parser. We don't clear the token
        // immediately
        // for Character tokens because the AtomicHTMLToken avoids copying the
        // characters by keeping a pointer to the underlying buffer in the
        // HTMLToken. Fortunately, Character tokens can't cause us to re-enter
        // the parser.
        //
        // FIXME: Stop clearing the rawToken once we start running the parser
        // off
        // the main thread or once we stop allowing synchronous JavaScript
        // execution from parseAttribute.
        if (rawToken.type() != HTMLToken::Character) {
            rawToken.clear();
        }

        m_treeBuilder.constructTree(&at);

        if (!rawToken.isUninitialized()) {
            STARFISH_ASSERT(rawToken.type() == HTMLToken::Character);
            rawToken.clear();
        }
    }
}
}
