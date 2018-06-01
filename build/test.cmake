CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# TIDY
#######################################################

ADD_CUSTOM_TARGET (tidy
    COMMAND python ${STARFISH_ROOT}/tool/check_tidy.py --path ${STARFISH_ROOT} > error_report
)

ADD_CUSTOM_TARGET (tidy-update
    COMMAND python ${STARFISH_ROOT}/tool/check_tidy.py -up ${STARFISH_ROOT}
)

#######################################################
# BACKEND
#######################################################

IF (${BACKEND} STREQUAL "efl_cairo")
    SET (TEST_BACKEND cairo)
ELSE()
    SET (TEST_BACKEND ${BACKEND})
ENDIF()

#######################################################
# TOOL BUILD 
#######################################################

ADD_EXECUTABLE (imgdiff EXCLUDE_FROM_ALL
    ${TOOL_ROOT}/imgdiff/imgdiff.cpp
)

SET_TARGET_PROPERTIES (imgdiff PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${STARFISH_ROOT}/out/tools"
)


pkg_check_modules (TEST_PNG REQUIRED libpng)
TARGET_LINK_LIBRARIES (imgdiff ${TEST_PNG_LIBRARIES})
TARGET_COMPILE_OPTIONS (imgdiff PUBLIC -O3 -g3 --std=c++11 ${TEST_PNG_CFLAGS})

ADD_CUSTOM_TARGET (install_pixel_test_dep
    DEPENDS imgdiff
    COMMAND ${CMAKE_COMMAND} -E make_directory ~/.fonts}
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/StarFishAhem.ttf ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/SamsungOne-300C_v1.0.ttf ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/SamsungOne-600C_v1.0.ttf ~/.fonts
    COMMAND fc-cache -fv
    COMMAND fc-match SamsungOne
)

#######################################################
# TEST TARGETS
#######################################################

ADD_CUSTOM_TARGET (dom_conformance_test
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test_webkit
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/webkit_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test_blink
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/blink_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test_gecko
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/gecko_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_all
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/dom_conformance_test.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/webkit_dom_conformance_test.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/blink_dom_conformance_test.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/gecko_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_dom
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_dom_events
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_html
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_page_visibility
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_progress_events
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_xhr
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (web_platform_test_css_cairo
    COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt_css.res common -p${NPROCS}
)

IF (${BACKEND} STREQUAL "efl_cairo")
    ADD_CUSTOM_TARGET (web_platform_test_all
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt_css.res common -p${NPROCS}
    )
ELSE()
    ADD_CUSTOM_TARGET (web_platform_test_all
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
    )
ENDIF()

ADD_CUSTOM_TARGET (vendor_test_blink_fast_dom
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_dom.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_html
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_html.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_css
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_etc
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_gecko_layout
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_dom
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_dom.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_html
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_html.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_css
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_etc
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_all
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_dom.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_html.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_dom.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_html.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (bidi_test
    COMMAND python ./tool/drivers/run_test.py bidi tool/reftest/${TEST_BACKEND}/bidi.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css1
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css21
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css21_tables
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css3_color
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css3_backgrounds
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css3_transforms
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_css3_selectors
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_mediaqueries3
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_manual
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_rtl
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_rtl.res ${TEST_BACKEND} -p${NPROCS}
)

ADD_CUSTOM_TARGET (csswg_test_flex_cairo
    COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_flex.res cairo -p${NPROCS}
)

IF (${BACKEND} STREQUAL "efl_cairo")
    ADD_CUSTOM_TARGET (csswg_test_all
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_flex.res cairo -p${NPROCS}
    )
ELSE()
    ADD_CUSTOM_TARGET (csswg_test_all
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    )
ENDIF()

ADD_CUSTOM_TARGET (internal_test
    COMMAND python ./tool/drivers/run_test.py basic tool/reftest/${TEST_BACKEND}/internal.res common -p${NPROCS}
    COMMAND python ./tool/drivers/run_test.py basic tool/reftest/${TEST_BACKEND}/internal_manual.res common --font-dep -p${NPROCS}
)

IF (${BACKEND} STREQUAL "efl_cairo")
    ADD_CUSTOM_TARGET (reftest_all
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/webkit_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/blink_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/gecko_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt_css.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_flex.res cairo -p${NPROCS}
    )
ELSE()
    ADD_CUSTOM_TARGET (reftest_all
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/webkit_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/blink_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/gecko_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    )
ENDIF()

IF (${BACKEND} STREQUAL "efl_cairo")
    ADD_CUSTOM_TARGET (test_all
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/webkit_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/blink_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/gecko_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt_css.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/cairo/tclist/csswg_flex.res cairo -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py basic tool/reftest/${TEST_BACKEND}/internal.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py basic tool/reftest/${TEST_BACKEND}/internal_manual.res common --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py bidi tool/reftest/${TEST_BACKEND}/bidi.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    )
ELSE()
    ADD_CUSTOM_TARGET (test_all
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/webkit_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/blink_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py dom_conformance tool/reftest/${TEST_BACKEND}/gecko_dom_conformance_test.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_dom_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_page_visibility.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_progress_events.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py multi_basic tool/reftest/${TEST_BACKEND}/wpt_xhr.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/blink_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/blink_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/gecko_layout_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_dom.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_basic tool/reftest/${TEST_BACKEND}/webkit_fast_html.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_css_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py vendor_pixel tool/reftest/${TEST_BACKEND}/webkit_fast_etc_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css1.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css21_tables.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_color.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_transforms.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_backgrounds.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_css3_selectors.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_mediaqueries3.res ${TEST_BACKEND} -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py csswg tool/reftest/${TEST_BACKEND}/tclist/csswg_manual.res ${TEST_BACKEND} --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py basic tool/reftest/${TEST_BACKEND}/internal.res common -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py basic tool/reftest/${TEST_BACKEND}/internal_manual.res common --font-dep -p${NPROCS}
        COMMAND python ./tool/drivers/run_test.py bidi tool/reftest/${TEST_BACKEND}/bidi.res ${TEST_BACKEND} --font-dep -p${NPROCS}
    )
ENDIF()

ADD_CUSTOM_TARGET (wpt_syntax_checker
    COMMAND ./tool/pixel_test/syntaxChecker.sh css1
    COMMAND ${CMAKE_COMMAND} -E echo "[wpt_syntax_checker] Updated tool/pixel_test/css1.res"
    COMMAND ./tool/pixel_test/syntaxChecker.sh css21
    COMMAND ${CMAKE_COMMAND} -E echo "[wpt_syntax_checker] Updated tool/pixel_test/css21.res"
    COMMAND ./tool/pixel_test/syntaxChecker.sh css-backgrounds-3
    COMMAND ${CMAKE_COMMAND} -E echo "[wpt_syntax_checker] Updated tool/pixel_test/css-backgrounds-3.res"
    COMMAND ./tool/pixel_test/syntaxChecker.sh css-color-3
    COMMAND ${CMAKE_COMMAND} -E echo "[wpt_syntax_checker] Updated tool/pixel_test/css-color-3.res"
    COMMAND ./tool/pixel_test/syntaxChecker.sh css-transforms-1
    COMMAND ${CMAKE_COMMAND} -E echo "[wpt_syntax_checker] Updated tool/pixel_test/css-transforms-1.res"
    COMMAND ${CMAKE_COMMAND} -E echo "[wpt_syntax_checker] COMPLETE.."
)

ADD_CUSTOM_TARGET (react_test_generate
    COMMAND ./generator.py WORKING_DIRECTORY ./test/cairo/reftest/vendor/react/
)

ADD_CUSTOM_TARGET (react_test
    DEPENDS react_test_generate
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/react.res common -p${NPROCS}
)
