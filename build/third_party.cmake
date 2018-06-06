CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# THIRD PARTY
#######################################################

# JS BINDING

EXECUTE_PROCESS (
    COMMAND python ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${STARFISH_ROOT}/src/binding
)
FILE (GLOB STARFISH_BINDING_OUTPUT_LIST ${STARFISH_ROOT}/src/binding/*.cpp)
#SET (STARFISH_BINDING_OUTPUT_LIST
#    ${STARFISH_ROOT}/src/binding/ArrayBufferViewOrArrayBufferBinding.cpp
#    ${STARFISH_ROOT}/src/binding/AttrBinding.cpp
#    ${STARFISH_ROOT}/src/binding/BlobBinding.cpp
#    ${STARFISH_ROOT}/src/binding/BlobCustomBinding.cpp
#    ${STARFISH_ROOT}/src/binding/BufferSourceOrBlobOrDOMStringBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CanvasGradientBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CanvasPatternBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CanvasRenderingContext2DBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CanvasRenderingContext2DOrWebGLRenderingContextOrImageBitmapRenderingContextBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CDATASectionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CharacterDataBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CharacterDataCustomBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CommentBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CompositionEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CompositionEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CoordinatesBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSConditionRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSCounterStyleRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSFontFaceRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSGroupingRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSImportRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSKeyframesRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSKeywordValueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSMediaRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSNamespaceRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSNumericTypeBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSNumericValueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSRuleListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSStyleDeclarationBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSStyleRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSStyleSheetBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSStyleValueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSSupportsRuleBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CSSUnitValueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CustomEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/CustomEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DocumentBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DocumentCustomBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DocumentFragmentBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DocumentHoldable.cpp
#    ${STARFISH_ROOT}/src/binding/DocumentTypeBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMExceptionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMImplementationBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMParserBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMPointBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMPointInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMPointReadOnlyBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMQuadBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMRectBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMRectListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMRectReadOnlyBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMSettableTokenListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMStringListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMStringMapBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMStringOrCanvasGradientOrCanvasPatternBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMStringOrFunctionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/DOMTokenListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/doubleOrAutoKeywordBinding.cpp
#    ${STARFISH_ROOT}/src/binding/doubleOrCSSNumericValueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ErrorEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ErrorEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventModifierInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventOrDOMStringBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventSourceBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventSourceInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/EventTargetBinding.cpp
#    ${STARFISH_ROOT}/src/binding/FocusEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/FocusEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/GeolocationBinding.cpp
#    ${STARFISH_ROOT}/src/binding/GeolocationCustomBinding.cpp
#    ${STARFISH_ROOT}/src/binding/GeopositionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HistoryBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLAnchorElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLAreaElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLAudioElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLBaseElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLBodyElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLBRElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLButtonElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLCanvasElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLCollectionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLDivElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLDListElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLDocumentBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLElementOrlongBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLFieldSetElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLFontElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLFormControlsCollectionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLFormElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLHeadElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLHeadingElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLHRElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLHtmlElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLIFrameElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLImageElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLInputElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLInputElementCustomBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLLabelElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLLegendElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLLIElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLLinkElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLMapElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLMediaElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLMetaElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLModElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLObjectElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLOListElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLOptGroupElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLOptionElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLOptionElementOrHTMLOptGroupElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLOptionsCollectionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLOutputElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLParagraphElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLParamElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLPreElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLQuoteElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLScriptElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLScriptElementOrSVGScriptElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLSelectElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLSourceElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLSpanElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLStyleElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTableCaptionElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTableCellElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTableColElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTableElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTableRowElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTableSectionElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTextAreaElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTFootElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTHeadElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTHElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTitleElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLTrackElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLUListElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLUnknownElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/HTMLVideoElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ImageBitmapRenderingContextBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ImageDataBinding.cpp
#    ${STARFISH_ROOT}/src/binding/InputEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/InputEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/KeyboardEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/KeyboardEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/LocationBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MediaListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MediaQueryListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MediaQueryListEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MediaQueryListEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MediaSourceBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MessageChannelBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MessageEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MessageEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MessagePortBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MouseEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/MouseEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NamedNodeMapBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NavigatorBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NodeBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NodeFilterBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NodeIteratorBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NodeListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/NodeOrDOMStringBinding.cpp
#    ${STARFISH_ROOT}/src/binding/Path2DBinding.cpp
#    ${STARFISH_ROOT}/src/binding/PositionErrorBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ProcessingInstructionBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ProgressEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ProgressEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/RangeBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ScreenBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ScriptBindingInstance.cpp
#    ${STARFISH_ROOT}/src/binding/ScriptEngineInstance.cpp
#    ${STARFISH_ROOT}/src/binding/ScriptWrappable.cpp
#    ${STARFISH_ROOT}/src/binding/ScrollOptionsBinding.cpp
#    ${STARFISH_ROOT}/src/binding/ScrollToOptionsBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SourceBufferBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SourceBufferListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/StorageBinding.cpp
#    ${STARFISH_ROOT}/src/binding/StyleSheetBinding.cpp
#    ${STARFISH_ROOT}/src/binding/StyleSheetListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGAnimatedLengthBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGCircleElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGDocumentBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGGElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGImageElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGLengthBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGPathElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGPolygonElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGPolylineElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGRectElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGScriptElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGStyleElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGSVGElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/SVGTextElementBinding.cpp
#    ${STARFISH_ROOT}/src/binding/testRunnerBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TextBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TextTrackBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TextTrackCueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TextTrackCueListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TextTrackListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TimeRangesBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TouchBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TouchEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TouchInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TouchListBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TransitionEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TransitionEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/TreeWalkerBinding.cpp
#    ${STARFISH_ROOT}/src/binding/UIEventBinding.cpp
#    ${STARFISH_ROOT}/src/binding/UIEventInitBinding.cpp
#    ${STARFISH_ROOT}/src/binding/URLBinding.cpp
#    ${STARFISH_ROOT}/src/binding/VTTCueBinding.cpp
#    ${STARFISH_ROOT}/src/binding/WebGLRenderingContextBinding.cpp
#    ${STARFISH_ROOT}/src/binding/WindowBinding.cpp
#    ${STARFISH_ROOT}/src/binding/WindowCustomBinding.cpp
#    ${STARFISH_ROOT}/src/binding/WindowHoldable.cpp
#    ${STARFISH_ROOT}/src/binding/XMLDocumentBinding.cpp
#    ${STARFISH_ROOT}/src/binding/XMLHttpRequestBinding.cpp
#    ${STARFISH_ROOT}/src/binding/XMLHttpRequestEventTargetBinding.cpp
#)

SET (THIRD_PARTY_CXXFLAGS_DEFAULT -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fno-omit-frame-pointer -fstack-protector -fPIC)

SET (THIRD_PARTY_CXXFLAGS ${THIRD_PARTY_CXXFLAGS_DEFAULT} ${LWE_CXXFLAGS_COMPILER} ${LWE_CXXFLAGS_MODE})

# SKIA
FILE (GLOB SKIA_LIST ${THIRD_PARTY_ROOT}/skia_matrix/*.cpp)
ADD_LIBRARY (skia SHARED ${SKIA_LIST})
TARGET_INCLUDE_DIRECTORIES (skia PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix)
TARGET_COMPILE_DEFINITIONS (skia PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (skia PUBLIC ${THIRD_PARTY_CXXFLAGS})

# CLIPPER
ADD_LIBRARY (clipper SHARED ${THIRD_PARTY_ROOT}/clipper/cpp/clipper.cpp)
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (clipper PUBLIC ${THIRD_PARTY_CXXFLAGS})

# MP4PARSE
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PUBLIC ${THIRD_PARTY_CXXFLAGS})

# WEBM
ADD_LIBRARY (webm SHARED
    ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
    ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
)
TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)
TARGET_COMPILE_DEFINITIONS (webm PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (webm PUBLIC ${THIRD_PARTY_CXXFLAGS})

#######################################################
# PREPARE THIRD PARTY LIBRARY FILES
#######################################################

IF (${ARCH} STREQUAL "tizen")
    SET (TUV_TARGET ${THIRD_PARTY_ROOT}/libtuv/build/noarch-tizen/debug/lib/libtuv.so)
    SET (ESCARGOT_TARGET ${ESCARGOT_ROOT}/libescargot.a)
    SET (GC_TARGET ${GCUTIL_ROOT}/bdwgc/out/tizen_obs/arm/${MODE}.shared/.libs/libgc.a)

    SET (TUV_LIB "${OUTPUT_DIRECTORY}/lib/libtuv.so")
    SET (ESCARGOT_LIB "${OUTPUT_DIRECTORY}/lib/libescargot.a")
    SET (GC_LIB "${OUTPUT_DIRECTORY}/lib/libgc.a")

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_TARGET} ${ESCARGOT_TARGET} ${GC_TARGET}
                        COMMENT "Build Third_Party for arm"
                        COMMAND ./build_third_party.sh arm
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_LIB} ${ESCARGOT_LIB} ${GC_LIB}
                        DEPENDS ${TUV_TARGET} ${ESCARGOT_TARGET} ${GC_TARGET}
                        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/libtuv/build/noarch-tizen/release/lib/libtuv.so ${OUTPUT_DIRECTORY}/lib
                        COMMAND ${CMAKE_COMMAND} -E copy ${ESCARGOT_ROOT}/libescargot.a ${OUTPUT_DIRECTORY}/lib
                        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/tizen_obs/arm/${MODE}.shared/.libs/libgc.a ${OUTPUT_DIRECTORY}/lib
    )

ELSE()
    SET (ZMQ_TARGET ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so.5 ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so.5.0.1)
    SET (ESCARGOT_TARGET ${ESCARGOT_ROOT}/out/${HOST}/${ARCH}/interpreter/${MODE}/libescargot.a)
    SET (GC_TARGET ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so.1 ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so.1.0.3)

    SET (ZMQ_LIB "${OUTPUT_DIRECTORY}/lib/libzmq.so")
    SET (ESCARGOT_LIB "${OUTPUT_DIRECTORY}/lib/libescargot.a")
    SET (GC_LIB "${OUTPUT_DIRECTORY}/lib/libgc.so")
    
    ADD_CUSTOM_COMMAND (OUTPUT ${ZMQ_TARGET} ${ESCARGOT_TARGET} ${GC_TARGET}
                        COMMENT "Build Third_Party for x64"
                        COMMAND ./build_third_party.sh x64
    )
                        
    ADD_CUSTOM_COMMAND (OUTPUT ${ZMQ_LIB} ${ESCARGOT_LIB} ${GC_LIB}
                        DEPENDS ${ZMQ_TARGET} ${ESCARGOT_TARGET} ${GC_TARGET}
                        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so ${OUTPUT_DIRECTORY}/lib/
                        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so.5 ${OUTPUT_DIRECTORY}/lib/
                        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so.5.0.1 ${OUTPUT_DIRECTORY}/lib/
                        COMMAND ${CMAKE_COMMAND} -E copy ${ESCARGOT_ROOT}/out/${HOST}/${ARCH}/interpreter/${MODE}/libescargot.a ${OUTPUT_DIRECTORY}/lib/
#                        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.a ${OUTPUT_DIRECTORY}/lib/
                        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so ${OUTPUT_DIRECTORY}/lib/
                        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so.1 ${OUTPUT_DIRECTORY}/lib/
                        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so.1.0.3 ${OUTPUT_DIRECTORY}/lib/
    )
ENDIF()

