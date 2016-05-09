#include "StarFishConfig.h"
#include "HTMLTokenizer.h"
#include "MarkupTokenizerInlines.h"
#include "HTMLEntityParser.h"
#include "AtomicHTMLToken.h"

namespace StarFish {


QualifiedName AtomicHTMLToken::nameForAttribute(const HTMLToken::Attribute& attribute) const
{
    return QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAttrAtomicString(m_starFish, new StringDataUTF32(attribute.name.data())));
}

bool AtomicHTMLToken::usesName() const
{
    return m_type == HTMLToken::StartTag || m_type == HTMLToken::EndTag || m_type == HTMLToken::DOCTYPE;
}

bool AtomicHTMLToken::usesAttributes() const
{
    return m_type == HTMLToken::StartTag || m_type == HTMLToken::EndTag;
}

static inline char32_t toLowerCase(char32_t cc)
{
    STARFISH_ASSERT(isASCIIUpper(cc));
    const int lowerCaseOffset = 0x20;
    return cc + lowerCaseOffset;
}

static inline bool vectorEqualsString(const std::vector<char, gc_allocator<char>>& vector, String* string)
{
    if (vector.size() != string->length())
        return false;

    if (!string->length())
        return true;

    return string->equals(vector.data());
}

static inline bool isEndTagBufferingState(HTMLTokenizer::State state)
{
    switch (state) {
    case HTMLTokenizer::RCDATAEndTagOpenState:
    case HTMLTokenizer::RCDATAEndTagNameState:
    case HTMLTokenizer::RAWTEXTEndTagOpenState:
    case HTMLTokenizer::RAWTEXTEndTagNameState:
    case HTMLTokenizer::ScriptDataEndTagOpenState:
    case HTMLTokenizer::ScriptDataEndTagNameState:
    case HTMLTokenizer::ScriptDataEscapedEndTagOpenState:
    case HTMLTokenizer::ScriptDataEscapedEndTagNameState:
        return true;
    default:
        return false;
    }
}

#define HTML_BEGIN_STATE(stateName) BEGIN_STATE(HTMLTokenizer, stateName)
#define HTML_RECONSUME_IN(stateName) RECONSUME_IN(HTMLTokenizer, stateName)
#define HTML_ADVANCE_TO(stateName) ADVANCE_TO(HTMLTokenizer, stateName)
#define HTML_SWITCH_TO(stateName) SWITCH_TO(HTMLTokenizer, stateName)

HTMLTokenizer::HTMLTokenizer()
    : m_inputStreamPreprocessor(this)
{
    reset();
}

HTMLTokenizer::~HTMLTokenizer()
{
}

void HTMLTokenizer::reset()
{
    m_state = HTMLTokenizer::DataState;
    m_token = 0;
    m_forceNullCharacterReplacement = false;
    m_shouldAllowCDATA = false;
    m_additionalAllowedCharacter = '\0';
}

bool HTMLTokenizer::canCreateCheckpoint() const
{
    if (!(m_appropriateEndTagName.size() == 0))
        return false;
    if (!(m_temporaryBuffer.size() == 0))
        return false;
    if (!(m_bufferedEndTagName.size() == 0))
        return false;
    return true;
}

void HTMLTokenizer::createCheckpoint(Checkpoint& result) const
{
    STARFISH_ASSERT(canCreateCheckpoint());
    result.state = m_state;
    result.additionalAllowedCharacter = m_additionalAllowedCharacter;
    result.skipNextNewLine = m_inputStreamPreprocessor.skipNextNewLine();
    result.shouldAllowCDATA = m_shouldAllowCDATA;
}

void HTMLTokenizer::restoreFromCheckpoint(const Checkpoint& checkpoint)
{
    m_token = 0;
    m_state = checkpoint.state;
    m_additionalAllowedCharacter = checkpoint.additionalAllowedCharacter;
    m_inputStreamPreprocessor.reset(checkpoint.skipNextNewLine);
    m_shouldAllowCDATA = checkpoint.shouldAllowCDATA;
}

inline bool HTMLTokenizer::processEntity(SegmentedString& source)
{
    bool notEnoughCharacters = false;
    DecodedHTMLEntity decodedEntity;
    bool success = consumeHTMLEntity(source, decodedEntity, notEnoughCharacters);
    if (notEnoughCharacters)
        return false;
    if (!success) {
        STARFISH_ASSERT(decodedEntity.isEmpty());
        bufferCharacter('&');
    } else {
        for (unsigned i = 0; i < decodedEntity.length; ++i)
            bufferCharacter(decodedEntity.data[i]);
    }
    return true;
}

bool HTMLTokenizer::flushBufferedEndTag(SegmentedString& source)
{
    STARFISH_ASSERT(m_token->type() == HTMLToken::Character || m_token->type() == HTMLToken::Uninitialized);
    source.advanceAndUpdateLineNumber();
    if (m_token->type() == HTMLToken::Character)
        return true;
    m_token->beginEndTag(m_bufferedEndTagName);
    m_bufferedEndTagName.clear();
    m_appropriateEndTagName.clear();
    m_temporaryBuffer.clear();
    return false;
}

#define FLUSH_AND_ADVANCE_TO(stateName)                                    \
    do {                                                                   \
        m_state = HTMLTokenizer::stateName;                           \
        if (flushBufferedEndTag(source))                                   \
            return true;                                                   \
        if (source.isEmpty()                                               \
            || !m_inputStreamPreprocessor.peek(source))                    \
            return haveBufferedCharacterToken();                           \
        cc = m_inputStreamPreprocessor.nextInputCharacter();               \
        goto stateName;                                                    \
    } while (false)

bool HTMLTokenizer::flushEmitAndResumeIn(SegmentedString& source, HTMLTokenizer::State state)
{
    m_state = state;
    flushBufferedEndTag(source);
    return true;
}

bool HTMLTokenizer::nextToken(SegmentedString& source, HTMLToken& token)
{
    // If we have a token in progress, then we're supposed to be called back
    // with the same token so we can finish it.
    STARFISH_ASSERT(!m_token || m_token == &token || token.type() == HTMLToken::Uninitialized);
    m_token = &token;

    if (!(m_bufferedEndTagName.size() == 0) && !isEndTagBufferingState(m_state)) {
        // FIXME: This should call flushBufferedEndTag().
        // We started an end tag during our last iteration.
        m_token->beginEndTag(m_bufferedEndTagName);
        m_bufferedEndTagName.clear();
        m_appropriateEndTagName.clear();
        m_temporaryBuffer.clear();
        if (m_state == HTMLTokenizer::DataState) {
            // We're back in the data state, so we must be done with the tag.
            return true;
        }
    }

    if (source.isEmpty() || !m_inputStreamPreprocessor.peek(source))
        return haveBufferedCharacterToken();
    char32_t cc = m_inputStreamPreprocessor.nextInputCharacter();

    // Source: http://www.whatwg.org/specs/web-apps/current-work/#tokenisation0
    switch (m_state) {
        HTML_BEGIN_STATE(DataState) {
        if (cc == '&')
            HTML_ADVANCE_TO(CharacterReferenceInDataState);
        else if (cc == '<') {
            if (m_token->type() == HTMLToken::Character) {
                // We have a bunch of character tokens queued up that we
                // are emitting lazily here.
                return true;
            }
            HTML_ADVANCE_TO(TagOpenState);
        } else if (cc == kEndOfFileMarker)
            return emitEndOfFile(source);
        else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(DataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CharacterReferenceInDataState) {
        if (!processEntity(source))
            return haveBufferedCharacterToken();
        HTML_SWITCH_TO(DataState);
    }
    END_STATE()

    HTML_BEGIN_STATE(RCDATAState) {
        if (cc == '&')
            HTML_ADVANCE_TO(CharacterReferenceInRCDATAState);
        else if (cc == '<')
            HTML_ADVANCE_TO(RCDATALessThanSignState);
        else if (cc == kEndOfFileMarker)
            return emitEndOfFile(source);
        else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(RCDATAState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CharacterReferenceInRCDATAState) {
        if (!processEntity(source))
            return haveBufferedCharacterToken();
        HTML_SWITCH_TO(RCDATAState);
    }
    END_STATE()

    HTML_BEGIN_STATE(RAWTEXTState) {
        if (cc == '<')
            HTML_ADVANCE_TO(RAWTEXTLessThanSignState);
        else if (cc == kEndOfFileMarker)
            return emitEndOfFile(source);
        else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(RAWTEXTState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataState) {
        if (cc == '<')
            HTML_ADVANCE_TO(ScriptDataLessThanSignState);
        else if (cc == kEndOfFileMarker)
            return emitEndOfFile(source);
        else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(PLAINTEXTState) {
        if (cc == kEndOfFileMarker)
            return emitEndOfFile(source);
        bufferCharacter(cc);
        HTML_ADVANCE_TO(PLAINTEXTState);
    }
    END_STATE()

    HTML_BEGIN_STATE(TagOpenState) {
        if (cc == '!')
            HTML_ADVANCE_TO(MarkupDeclarationOpenState);
        else if (cc == '/')
            HTML_ADVANCE_TO(EndTagOpenState);
        else if (isASCIIUpper(cc)) {
            m_token->beginStartTag(toLowerCase(cc));
            HTML_ADVANCE_TO(TagNameState);
        } else if (isASCIILower(cc)) {
            m_token->beginStartTag(cc);
            HTML_ADVANCE_TO(TagNameState);
        } else if (cc == '?') {
            parseError();
            // The spec consumes the current character before switching
            // to the bogus comment state, but it's easier to implement
            // if we reconsume the current character.
            HTML_RECONSUME_IN(BogusCommentState);
        } else {
            parseError();
            bufferCharacter('<');
            HTML_RECONSUME_IN(DataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(EndTagOpenState) {
        if (isASCIIUpper(cc)) {
            m_token->beginEndTag(static_cast<char>(toLowerCase(cc)));
            m_appropriateEndTagName.clear();
            HTML_ADVANCE_TO(TagNameState);
        } else if (isASCIILower(cc)) {
            m_token->beginEndTag(static_cast<char>(cc));
            m_appropriateEndTagName.clear();
            HTML_ADVANCE_TO(TagNameState);
        } else if (cc == '>') {
            parseError();
            HTML_ADVANCE_TO(DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            bufferCharacter('<');
            bufferCharacter('/');
            HTML_RECONSUME_IN(DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BogusCommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(TagNameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            m_token->appendToName(toLowerCase(cc));
            HTML_ADVANCE_TO(TagNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            m_token->appendToName(cc);
            HTML_ADVANCE_TO(TagNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(RCDATALessThanSignState) {
        if (cc == '/') {
            m_temporaryBuffer.clear();
            STARFISH_ASSERT(m_bufferedEndTagName.size() == 0);
            HTML_ADVANCE_TO(RCDATAEndTagOpenState);
        } else {
            bufferCharacter('<');
            HTML_RECONSUME_IN(RCDATAState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(RCDATAEndTagOpenState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(RCDATAEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(RCDATAEndTagNameState);
        } else {
            bufferCharacter('<');
            bufferCharacter('/');
            HTML_RECONSUME_IN(RCDATAState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(RCDATAEndTagNameState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(RCDATAEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(RCDATAEndTagNameState);
        } else {
            if (isTokenizerWhitespace(cc)) {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(BeforeAttributeNameState);
                }
            } else if (cc == '/') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(SelfClosingStartTagState);
                }
            } else if (cc == '>') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    return flushEmitAndResumeIn(source, HTMLTokenizer::DataState);
                }
            }
            bufferCharacter('<');
            bufferCharacter('/');
            m_token->appendToCharacter(m_temporaryBuffer);
            m_bufferedEndTagName.clear();
            m_temporaryBuffer.clear();
            HTML_RECONSUME_IN(RCDATAState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(RAWTEXTLessThanSignState) {
        if (cc == '/') {
            m_temporaryBuffer.clear();
            STARFISH_ASSERT(m_bufferedEndTagName.size() == 0);
            HTML_ADVANCE_TO(RAWTEXTEndTagOpenState);
        } else {
            bufferCharacter('<');
            HTML_RECONSUME_IN(RAWTEXTState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(RAWTEXTEndTagOpenState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(RAWTEXTEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(RAWTEXTEndTagNameState);
        } else {
            bufferCharacter('<');
            bufferCharacter('/');
            HTML_RECONSUME_IN(RAWTEXTState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(RAWTEXTEndTagNameState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(RAWTEXTEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(RAWTEXTEndTagNameState);
        } else {
            if (isTokenizerWhitespace(cc)) {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(BeforeAttributeNameState);
                }
            } else if (cc == '/') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(SelfClosingStartTagState);
                }
            } else if (cc == '>') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    return flushEmitAndResumeIn(source, HTMLTokenizer::DataState);
                }
            }
            bufferCharacter('<');
            bufferCharacter('/');
            m_token->appendToCharacter(m_temporaryBuffer);
            m_bufferedEndTagName.clear();
            m_temporaryBuffer.clear();
            HTML_RECONSUME_IN(RAWTEXTState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataLessThanSignState) {
        if (cc == '/') {
            m_temporaryBuffer.clear();
            STARFISH_ASSERT(m_bufferedEndTagName.size() == 0);
            HTML_ADVANCE_TO(ScriptDataEndTagOpenState);
        } else if (cc == '!') {
            bufferCharacter('<');
            bufferCharacter('!');
            HTML_ADVANCE_TO(ScriptDataEscapeStartState);
        } else {
            bufferCharacter('<');
            HTML_RECONSUME_IN(ScriptDataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEndTagOpenState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(ScriptDataEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataEndTagNameState);
        } else {
            bufferCharacter('<');
            bufferCharacter('/');
            HTML_RECONSUME_IN(ScriptDataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEndTagNameState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(ScriptDataEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataEndTagNameState);
        } else {
            if (isTokenizerWhitespace(cc)) {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(BeforeAttributeNameState);
                }
            } else if (cc == '/') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(SelfClosingStartTagState);
                }
            } else if (cc == '>') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    return flushEmitAndResumeIn(source, HTMLTokenizer::DataState);
                }
            }
            bufferCharacter('<');
            bufferCharacter('/');
            m_token->appendToCharacter(m_temporaryBuffer);
            m_bufferedEndTagName.clear();
            m_temporaryBuffer.clear();
            HTML_RECONSUME_IN(ScriptDataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapeStartState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapeStartDashState);
        } else
            HTML_RECONSUME_IN(ScriptDataState);
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapeStartDashState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedDashDashState);
        } else
            HTML_RECONSUME_IN(ScriptDataState);
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapedState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedDashState);
        } else if (cc == '<')
            HTML_ADVANCE_TO(ScriptDataEscapedLessThanSignState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapedDashState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedDashDashState);
        } else if (cc == '<')
            HTML_ADVANCE_TO(ScriptDataEscapedLessThanSignState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapedDashDashState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedDashDashState);
        } else if (cc == '<')
            HTML_ADVANCE_TO(ScriptDataEscapedLessThanSignState);
        else if (cc == '>') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapedLessThanSignState) {
        if (cc == '/') {
            m_temporaryBuffer.clear();
            STARFISH_ASSERT(m_bufferedEndTagName.size() == 0);
            HTML_ADVANCE_TO(ScriptDataEscapedEndTagOpenState);
        } else if (isASCIIUpper(cc)) {
            bufferCharacter('<');
            bufferCharacter(cc);
            m_temporaryBuffer.clear();
            m_temporaryBuffer.push_back(toLowerCase(cc));
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeStartState);
        } else if (isASCIILower(cc)) {
            bufferCharacter('<');
            bufferCharacter(cc);
            m_temporaryBuffer.clear();
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeStartState);
        } else {
            bufferCharacter('<');
            HTML_RECONSUME_IN(ScriptDataEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapedEndTagOpenState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(ScriptDataEscapedEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataEscapedEndTagNameState);
        } else {
            bufferCharacter('<');
            bufferCharacter('/');
            HTML_RECONSUME_IN(ScriptDataEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataEscapedEndTagNameState) {
        if (isASCIIUpper(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(toLowerCase(cc)));
            HTML_ADVANCE_TO(ScriptDataEscapedEndTagNameState);
        } else if (isASCIILower(cc)) {
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            addToPossibleEndTag(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataEscapedEndTagNameState);
        } else {
            if (isTokenizerWhitespace(cc)) {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(BeforeAttributeNameState);
                }
            } else if (cc == '/') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    FLUSH_AND_ADVANCE_TO(SelfClosingStartTagState);
                }
            } else if (cc == '>') {
                if (isAppropriateEndTag()) {
                    m_temporaryBuffer.push_back(static_cast<char>(cc));
                    return flushEmitAndResumeIn(source, HTMLTokenizer::DataState);
                }
            }
            bufferCharacter('<');
            bufferCharacter('/');
            m_token->appendToCharacter(m_temporaryBuffer);
            m_bufferedEndTagName.clear();
            m_temporaryBuffer.clear();
            HTML_RECONSUME_IN(ScriptDataEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataDoubleEscapeStartState) {
        if (isTokenizerWhitespace(cc) || cc == '/' || cc == '>') {
            bufferCharacter(cc);
            if (temporaryBufferIs("script"))
                HTML_ADVANCE_TO(ScriptDataDoubleEscapedState);
            else
                HTML_ADVANCE_TO(ScriptDataEscapedState);
        } else if (isASCIIUpper(cc)) {
            bufferCharacter(cc);
            m_temporaryBuffer.push_back(toLowerCase(cc));
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeStartState);
        } else if (isASCIILower(cc)) {
            bufferCharacter(cc);
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeStartState);
        } else
            HTML_RECONSUME_IN(ScriptDataEscapedState);
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataDoubleEscapedState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedDashState);
        } else if (cc == '<') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedLessThanSignState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataDoubleEscapedDashState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedDashDashState);
        } else if (cc == '<') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedLessThanSignState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataDoubleEscapedDashDashState) {
        if (cc == '-') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedDashDashState);
        } else if (cc == '<') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedLessThanSignState);
        } else if (cc == '>') {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(ScriptDataDoubleEscapedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataDoubleEscapedLessThanSignState) {
        if (cc == '/') {
            bufferCharacter(cc);
            m_temporaryBuffer.clear();
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeEndState);
        } else
            HTML_RECONSUME_IN(ScriptDataDoubleEscapedState);
    }
    END_STATE()

    HTML_BEGIN_STATE(ScriptDataDoubleEscapeEndState) {
        if (isTokenizerWhitespace(cc) || cc == '/' || cc == '>') {
            bufferCharacter(cc);
            if (temporaryBufferIs("script"))
                HTML_ADVANCE_TO(ScriptDataEscapedState);
            else
                HTML_ADVANCE_TO(ScriptDataDoubleEscapedState);
        } else if (isASCIIUpper(cc)) {
            bufferCharacter(cc);
            m_temporaryBuffer.push_back(toLowerCase(cc));
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeEndState);
        } else if (isASCIILower(cc)) {
            bufferCharacter(cc);
            m_temporaryBuffer.push_back(static_cast<char>(cc));
            HTML_ADVANCE_TO(ScriptDataDoubleEscapeEndState);
        } else
            HTML_RECONSUME_IN(ScriptDataDoubleEscapedState);
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeAttributeNameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            m_token->addNewAttribute();
            m_token->beginAttributeName(source.numberOfCharactersConsumed());
            m_token->appendToAttributeName(toLowerCase(cc));
            HTML_ADVANCE_TO(AttributeNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<' || cc == '=')
                parseError();
            m_token->addNewAttribute();
            m_token->beginAttributeName(source.numberOfCharactersConsumed());
            m_token->appendToAttributeName(cc);
            HTML_ADVANCE_TO(AttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeNameState) {
        if (isTokenizerWhitespace(cc)) {
            m_token->endAttributeName(source.numberOfCharactersConsumed());
            HTML_ADVANCE_TO(AfterAttributeNameState);
        } else if (cc == '/') {
            m_token->endAttributeName(source.numberOfCharactersConsumed());
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        } else if (cc == '=') {
            m_token->endAttributeName(source.numberOfCharactersConsumed());
            HTML_ADVANCE_TO(BeforeAttributeValueState);
        } else if (cc == '>') {
            m_token->endAttributeName(source.numberOfCharactersConsumed());
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (isASCIIUpper(cc)) {
            m_token->appendToAttributeName(toLowerCase(cc));
            HTML_ADVANCE_TO(AttributeNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->endAttributeName(source.numberOfCharactersConsumed());
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<' || cc == '=')
                parseError();
            m_token->appendToAttributeName(cc);
            HTML_ADVANCE_TO(AttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterAttributeNameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '=')
            HTML_ADVANCE_TO(BeforeAttributeValueState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            m_token->addNewAttribute();
            m_token->beginAttributeName(source.numberOfCharactersConsumed());
            m_token->appendToAttributeName(toLowerCase(cc));
            HTML_ADVANCE_TO(AttributeNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<')
                parseError();
            m_token->addNewAttribute();
            m_token->beginAttributeName(source.numberOfCharactersConsumed());
            m_token->appendToAttributeName(cc);
            HTML_ADVANCE_TO(AttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeAttributeValueState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeValueState);
        else if (cc == '"') {
            m_token->beginAttributeValue(source.numberOfCharactersConsumed() + 1);
            HTML_ADVANCE_TO(AttributeValueDoubleQuotedState);
        } else if (cc == '&') {
            m_token->beginAttributeValue(source.numberOfCharactersConsumed());
            HTML_RECONSUME_IN(AttributeValueUnquotedState);
        } else if (cc == '\'') {
            m_token->beginAttributeValue(source.numberOfCharactersConsumed() + 1);
            HTML_ADVANCE_TO(AttributeValueSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '<' || cc == '=' || cc == '`')
                parseError();
            m_token->beginAttributeValue(source.numberOfCharactersConsumed());
            m_token->appendToAttributeValue(cc);
            HTML_ADVANCE_TO(AttributeValueUnquotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeValueDoubleQuotedState) {
        if (cc == '"') {
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            HTML_ADVANCE_TO(AfterAttributeValueQuotedState);
        } else if (cc == '&') {
            m_additionalAllowedCharacter = '"';
            HTML_ADVANCE_TO(CharacterReferenceInAttributeValueState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            HTML_RECONSUME_IN(DataState);
        } else {
            m_token->appendToAttributeValue(cc);
            HTML_ADVANCE_TO(AttributeValueDoubleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeValueSingleQuotedState) {
        if (cc == '\'') {
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            HTML_ADVANCE_TO(AfterAttributeValueQuotedState);
        } else if (cc == '&') {
            m_additionalAllowedCharacter = '\'';
            HTML_ADVANCE_TO(CharacterReferenceInAttributeValueState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            HTML_RECONSUME_IN(DataState);
        } else {
            m_token->appendToAttributeValue(cc);
            HTML_ADVANCE_TO(AttributeValueSingleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeValueUnquotedState) {
        if (isTokenizerWhitespace(cc)) {
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        } else if (cc == '&') {
            m_additionalAllowedCharacter = '>';
            HTML_ADVANCE_TO(CharacterReferenceInAttributeValueState);
        } else if (cc == '>') {
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->endAttributeValue(source.numberOfCharactersConsumed());
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<' || cc == '=' || cc == '`')
                parseError();
            m_token->appendToAttributeValue(cc);
            HTML_ADVANCE_TO(AttributeValueUnquotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CharacterReferenceInAttributeValueState) {
        bool notEnoughCharacters = false;
        DecodedHTMLEntity decodedEntity;
        bool success = consumeHTMLEntity(source, decodedEntity, notEnoughCharacters, m_additionalAllowedCharacter);
        if (notEnoughCharacters)
            return haveBufferedCharacterToken();
        if (!success) {
            STARFISH_ASSERT(decodedEntity.isEmpty());
            m_token->appendToAttributeValue('&');
        } else {
            for (unsigned i = 0; i < decodedEntity.length; ++i)
                m_token->appendToAttributeValue(decodedEntity.data[i]);
        }
        // We're supposed to switch back to the attribute value state that
        // we were in when we were switched into this state. Rather than
        // keeping track of this explictly, we observe that the previous
        // state can be determined by m_additionalAllowedCharacter.
        if (m_additionalAllowedCharacter == '"')
            HTML_SWITCH_TO(AttributeValueDoubleQuotedState);
        else if (m_additionalAllowedCharacter == '\'')
            HTML_SWITCH_TO(AttributeValueSingleQuotedState);
        else if (m_additionalAllowedCharacter == '>')
            HTML_SWITCH_TO(AttributeValueUnquotedState);
        else
            STARFISH_ASSERT_NOT_REACHED();
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterAttributeValueQuotedState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BeforeAttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(SelfClosingStartTagState) {
        if (cc == '>') {
            m_token->setSelfClosing();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BeforeAttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BogusCommentState) {
        m_token->beginComment();
        HTML_RECONSUME_IN(ContinueBogusCommentState);
    }
    END_STATE()

    HTML_BEGIN_STATE(ContinueBogusCommentState) {
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker)
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        else {
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(ContinueBogusCommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(MarkupDeclarationOpenState) {
        // DEFINE_STATIC_LOCAL(String, dashDashString, ("--"));
        // DEFINE_STATIC_LOCAL(String, doctypeString, ("doctype"));
        // DEFINE_STATIC_LOCAL(String, cdataString, ("[CDATA["));
        if (cc == '-') {
            SegmentedString::LookAheadResult result = source.lookAhead(String::createASCIIString("--"));
            if (result == SegmentedString::DidMatch) {
                source.advanceAndASSERT('-');
                source.advanceAndASSERT('-');
                m_token->beginComment();
                HTML_SWITCH_TO(CommentStartState);
            } else if (result == SegmentedString::NotEnoughCharacters)
                return haveBufferedCharacterToken();
        } else if (cc == 'D' || cc == 'd') {
            SegmentedString::LookAheadResult result = source.lookAheadIgnoringCase(String::createASCIIString("doctype"));
            if (result == SegmentedString::DidMatch) {
                advanceStringAndASSERTIgnoringCase(source, "doctype");
                HTML_SWITCH_TO(DOCTYPEState);
            } else if (result == SegmentedString::NotEnoughCharacters)
                return haveBufferedCharacterToken();
        } else if (cc == '[' && shouldAllowCDATA()) {
            SegmentedString::LookAheadResult result = source.lookAhead(String::createASCIIString("[CDATA["));
            if (result == SegmentedString::DidMatch) {
                advanceStringAndASSERT(source, "[CDATA[");
                HTML_SWITCH_TO(CDATASectionState);
            } else if (result == SegmentedString::NotEnoughCharacters)
                return haveBufferedCharacterToken();
        }
        parseError();
        HTML_RECONSUME_IN(BogusCommentState);
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentStartState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentStartDashState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentStartDashState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentEndState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToComment('-');
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentEndDashState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentEndDashState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentEndState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToComment('-');
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentEndState) {
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == '!') {
            parseError();
            HTML_ADVANCE_TO(CommentEndBangState);
        } else if (cc == '-') {
            parseError();
            m_token->appendToComment('-');
            HTML_ADVANCE_TO(CommentEndState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->appendToComment('-');
            m_token->appendToComment('-');
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentEndBangState) {
        if (cc == '-') {
            m_token->appendToComment('-');
            m_token->appendToComment('-');
            m_token->appendToComment('!');
            HTML_ADVANCE_TO(CommentEndDashState);
        } else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToComment('-');
            m_token->appendToComment('-');
            m_token->appendToComment('!');
            m_token->appendToComment(cc);
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPEState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPENameState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->beginDOCTYPE();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BeforeDOCTYPENameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeDOCTYPENameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPENameState);
        else if (isASCIIUpper(cc)) {
            m_token->beginDOCTYPE(toLowerCase(cc));
            HTML_ADVANCE_TO(DOCTYPENameState);
        } else if (cc == '>') {
            parseError();
            m_token->beginDOCTYPE();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->beginDOCTYPE();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->beginDOCTYPE(cc);
            HTML_ADVANCE_TO(DOCTYPENameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPENameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterDOCTYPENameState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            m_token->appendToName(toLowerCase(cc));
            HTML_ADVANCE_TO(DOCTYPENameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToName(cc);
            HTML_ADVANCE_TO(DOCTYPENameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPENameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterDOCTYPENameState);
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            // DEFINE_STATIC_LOCAL(String, publicString, ("public"));
            // DEFINE_STATIC_LOCAL(String, systemString, ("system"));
            if (cc == 'P' || cc == 'p') {
                SegmentedString::LookAheadResult result = source.lookAheadIgnoringCase(String::createASCIIString("public"));
                if (result == SegmentedString::DidMatch) {
                    advanceStringAndASSERTIgnoringCase(source, "public");
                    HTML_SWITCH_TO(AfterDOCTYPEPublicKeywordState);
                } else if (result == SegmentedString::NotEnoughCharacters)
                    return haveBufferedCharacterToken();
            } else if (cc == 'S' || cc == 's') {
                SegmentedString::LookAheadResult result = source.lookAheadIgnoringCase(String::createASCIIString("system"));
                if (result == SegmentedString::DidMatch) {
                    advanceStringAndASSERTIgnoringCase(source, "system");
                    HTML_SWITCH_TO(AfterDOCTYPESystemKeywordState);
                } else if (result == SegmentedString::NotEnoughCharacters)
                    return haveBufferedCharacterToken();
            }
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPEPublicKeywordState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPEPublicIdentifierState);
        else if (cc == '"') {
            parseError();
            m_token->setPublicIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            parseError();
            m_token->setPublicIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeDOCTYPEPublicIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPEPublicIdentifierState);
        else if (cc == '"') {
            m_token->setPublicIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            m_token->setPublicIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPEPublicIdentifierDoubleQuotedState) {
        if (cc == '"')
            HTML_ADVANCE_TO(AfterDOCTYPEPublicIdentifierState);
        else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToPublicIdentifier(cc);
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierDoubleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPEPublicIdentifierSingleQuotedState) {
        if (cc == '\'')
            HTML_ADVANCE_TO(AfterDOCTYPEPublicIdentifierState);
        else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToPublicIdentifier(cc);
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierSingleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPEPublicIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BetweenDOCTYPEPublicAndSystemIdentifiersState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == '"') {
            parseError();
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            parseError();
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BetweenDOCTYPEPublicAndSystemIdentifiersState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BetweenDOCTYPEPublicAndSystemIdentifiersState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == '"') {
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPESystemKeywordState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPESystemIdentifierState);
        else if (cc == '"') {
            parseError();
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            parseError();
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeDOCTYPESystemIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPESystemIdentifierState);
        if (cc == '"') {
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            m_token->setSystemIdentifierToEmptyString();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            m_token->setForceQuirks();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPESystemIdentifierDoubleQuotedState) {
        if (cc == '"')
            HTML_ADVANCE_TO(AfterDOCTYPESystemIdentifierState);
        else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToSystemIdentifier(cc);
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPESystemIdentifierSingleQuotedState) {
        if (cc == '\'')
            HTML_ADVANCE_TO(AfterDOCTYPESystemIdentifierState);
        else if (cc == '>') {
            parseError();
            m_token->setForceQuirks();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->appendToSystemIdentifier(cc);
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPESystemIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterDOCTYPESystemIdentifierState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->setForceQuirks();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BogusDOCTYPEState) {
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker)
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        HTML_ADVANCE_TO(BogusDOCTYPEState);
    }
    END_STATE()

    HTML_BEGIN_STATE(CDATASectionState) {
        if (cc == ']')
            HTML_ADVANCE_TO(CDATASectionRightSquareBracketState);
        else if (cc == kEndOfFileMarker)
            HTML_RECONSUME_IN(DataState);
        else {
            bufferCharacter(cc);
            HTML_ADVANCE_TO(CDATASectionState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CDATASectionRightSquareBracketState) {
        if (cc == ']')
            HTML_ADVANCE_TO(CDATASectionDoubleRightSquareBracketState);
        else {
            bufferCharacter(']');
            HTML_RECONSUME_IN(CDATASectionState);
        }
    }

    HTML_BEGIN_STATE(CDATASectionDoubleRightSquareBracketState) {
        if (cc == '>')
            HTML_ADVANCE_TO(DataState);
        else {
            bufferCharacter(']');
            bufferCharacter(']');
            HTML_RECONSUME_IN(CDATASectionState);
        }
    }
    END_STATE()

    }

    STARFISH_ASSERT_NOT_REACHED();
    return false;
}

String* HTMLTokenizer::bufferedCharacters() const
{
    // FIXME: Add an assert about m_state.
    ASCIIString characters;
    characters.push_back('<');
    characters.push_back('/');
    characters.append(m_temporaryBuffer.begin(), m_temporaryBuffer.end());
    return new StringDataASCII(std::move(characters));
}

void HTMLTokenizer::updateStateFor(String* tagName)
{
    if (tagName->equals("textarea") || tagName->equals("title")) {
        setState(HTMLTokenizer::RCDATAState);
    } else if (tagName->equals("plaintext")) {
        setState(HTMLTokenizer::PLAINTEXTState);
    } else if (tagName->equals("script")) {
        setState(HTMLTokenizer::ScriptDataState);
    } else if (tagName->equals("style")
        || tagName->equals("iframe")
        || tagName->equals("xmp")
        || tagName->equals("noembed")
        || tagName->equals("noframes")
        || tagName->equals("noscript")
            ) {
        setState(HTMLTokenizer::RAWTEXTState);
    }
}

inline bool HTMLTokenizer::temporaryBufferIs(String* expectedString)
{
    return expectedString->equals(m_temporaryBuffer.data());
}

inline bool HTMLTokenizer::temporaryBufferIs(const char* expectedString)
{
    return memcmp(m_temporaryBuffer.data(), expectedString, m_temporaryBuffer.size()) == 0;
}

inline void HTMLTokenizer::addToPossibleEndTag(char cc)
{
    STARFISH_ASSERT(isEndTagBufferingState(m_state));
    m_bufferedEndTagName.push_back(cc);
}

inline bool HTMLTokenizer::isAppropriateEndTag()
{
    if (m_bufferedEndTagName.size() != m_appropriateEndTagName.size())
        return false;

    size_t numCharacters = m_bufferedEndTagName.size();

    for (size_t i = 0; i < numCharacters; i++) {
        if ((char32_t)m_bufferedEndTagName[i] != m_appropriateEndTagName[i])
            return false;
    }

    return true;
}

inline void HTMLTokenizer::parseError()
{
    // STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

}
