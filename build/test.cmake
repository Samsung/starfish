CMAKE_MINIMUM_REQUIRED (VERSION 2.8)


#######################################################
# TOOL BUILD 
#######################################################

ADD_EXECUTABLE (imgdiff EXCLUDE_FROM_ALL
    ${TOOL_ROOT}/imgdiff/imgdiff.cpp
)

SET_TARGET_PROPERTIES (imgdiff PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TOOL_ROOT}/imgdiff"
)

pkg_check_modules (TEST_PNG REQUIRED libpng)
TARGET_LINK_LIBRARIES (imgdiff ${TEST_PNG_LIBRARIES})
TARGET_COMPILE_OPTIONS (imgdiff PUBLIC -O3 -g3 --std=c++11 ${TEST_PNG_CFLAGS})

ADD_CUSTOM_TARGET (install_pixel_test_dep
    DEPENDS imgdiff
    COMMAND ${CMAKE_COMMAND} -E make_directory ~/.fonts}
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/StarfishAhem.ttf ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/SamsungOne-300C_v1.0.ttf ~/.fonts
    COMMAND ${CMAKE_COMMAND} -E copy ${TOOL_ROOT}/fonts/SamsungOne-600C_v1.0.ttf ~/.fonts
    COMMAND fc-cache -fv
    COMMAND fc-match SamsungOne
)

#######################################################
# TEST TARGETS
#######################################################

ADD_CUSTOM_TARGET (internal_test
    COMMAND ./tool/drivers/run_test.py basic tool/reftest/cairo/internal.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py basic tool/reftest/cairo/internal_manual.res common --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/pixel_test/svg.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (internal_test_manual
    COMMAND ./tool/drivers/run_test.py basic tool/reftest/cairo/internal_manual.res common --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test_webkit
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/webkit_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test_blink
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/blink_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_test_gecko
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/gecko_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (dom_conformance_all
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/webkit_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/blink_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/gecko_dom_conformance_test.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_dom
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_dom.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_html
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_html.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_css
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_fast_etc
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_table.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_css3
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_css3.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_blink_svg
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_svg.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/blink_svg_basic.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_gecko_layout
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_dom
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_dom.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_html
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_html.res common -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_css
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_webkit_fast_etc
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (vendor_test_all
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_dom.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_html.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_table.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_css3.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_svg.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/blink_svg_basic.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_dom.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_html.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_css21
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css21_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_backgrounds
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_color
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-4_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_flexbox
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-flexbox-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_cssom_view
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_cssom-view-1_dev_basic.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_transforms
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-transforms-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_variables
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-variables-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_mediaqueries
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_selectors
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-3_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res cairo -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-4_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_css_all
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css21_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-4_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-flexbox-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_cssom-view-1_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-transforms-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-variables-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-3_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res cairo -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-4_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res cairo -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_others
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/html_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg_with_remote tool/reftest/cairo/wpt/html_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_parsing_basic.res basic -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_xpath_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/page_visibility_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_single_thread.res basic -p1
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/x-frame-options.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/csp.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/webstorage.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cors.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cookies.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/serviceworker.res basic -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_canvas
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/2dcontext.res basic -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_pwa
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/fetch_basic.res basic -p${NPROCS}
)

ADD_CUSTOM_TARGET (wpt_all
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css21_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-4_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-flexbox-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_cssom-view-1_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-transforms-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-variables-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-3_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res cairo -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-4_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/html_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg_with_remote tool/reftest/cairo/wpt/html_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_parsing_basic.res basic -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_xpath_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/page_visibility_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_single_thread.res basic -p1
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/x-frame-options.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/csp.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/webstorage.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cors.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cookies.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/serviceworker.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/2dcontext.res basic -p${NPROCS}
)

ADD_CUSTOM_TARGET (reftest_all
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/webkit_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/blink_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/gecko_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_dom.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_html.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_table.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_css3.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_svg.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/blink_svg_basic.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_dom.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_html.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc_manual.res cairo --font-dep -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css21_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-4_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-flexbox-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_cssom-view-1_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-transforms-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-variables-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-3_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res cairo -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-4_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/html_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg_with_remote tool/reftest/cairo/wpt/html_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_parsing_basic.res basic -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_xpath_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/page_visibility_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_single_thread.res basic -p1
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/x-frame-options.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/csp.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/webstorage.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cors.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cookies.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/2dcontext.res basic -p${NPROCS}
)

ADD_CUSTOM_TARGET (bidi_test
    COMMAND ./tool/drivers/run_test.py bidi tool/reftest/cairo/bidi.res cairo --font-dep -p${NPROCS}
)

ADD_CUSTOM_TARGET (test_all
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/webkit_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/blink_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py dom_conformance tool/reftest/cairo/gecko_dom_conformance_test.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_dom.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/blink_fast_html.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_css_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_etc_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_fast_table.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_css3.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/blink_svg.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/blink_svg_basic.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/gecko_layout_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_dom.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_basic tool/reftest/cairo/webkit_fast_html.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_css_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py vendor_pixel tool/reftest/cairo/webkit_fast_etc_manual.res cairo --font-dep -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css21_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css21_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-backgrounds-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-3_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-3_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-color-4_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-color-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-flexbox-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-flexbox-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_cssom-view-1_dev_basic.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-transforms-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-transforms-1_dev_manual.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_css-variables-1_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_css-variables-1_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_mediaqueries-3_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-3_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-3_dev_pixel.res cairo -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/css_selectors-4_dev_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/reftest/cairo/wpt/css_selectors-4_dev_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/html_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg_with_remote tool/reftest/cairo/wpt/html_pixel.res cairo -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_parsing_basic.res basic -p${NPROCS}
#    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/dom_xpath_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/page_visibility_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_basic.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/xhr_single_thread.res basic -p1
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/x-frame-options.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/csp.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/webstorage.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cors.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py multi_basic tool/reftest/cairo/wpt/cookies.res basic -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py bidi tool/reftest/cairo/bidi.res cairo --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py basic tool/reftest/cairo/internal.res common -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py basic tool/reftest/cairo/internal_manual.res common --font-dep -p${NPROCS}
    COMMAND ./tool/drivers/run_test.py csswg tool/pixel_test/svg.res cairo -p${NPROCS}
)

