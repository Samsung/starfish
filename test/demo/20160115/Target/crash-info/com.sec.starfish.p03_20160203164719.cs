S/W Version Information
Model: SM-R720
Tizen-Version: 2.3.1.2
Build-Number: R720XXE2BOKH
Build-Date: 2015.11.24 18:54:57

Crash Information
Process Name: p03
PID: 21538
Date: 2016-02-03 16:47:19+0900
Executable File Path: /opt/usr/apps/com.sec.starfish.p03/bin/p03
Signal: 11
      (SIGSEGV)
      si_code: 1
      address not mapped to object
      si_addr = 0x108

Register Information
r0   = 0x00000000, r1   = 0x43e2aa20
r2   = 0x00000001, r3   = 0x43e3a800
r4   = 0x43e2aa20, r5   = 0x401039f0
r6   = 0x00000001, r7   = 0x00000000
r8   = 0x40107988, r9   = 0x40107988
r10  = 0x40107988, fp   = 0x00000000
ip   = 0x40103bf0, sp   = 0xbee12e38
lr   = 0x4007e7ed, pc   = 0x4009ce30
cpsr = 0x00000030

Memory Information
MemTotal:   489820 KB
MemFree:     13540 KB
Buffers:     49252 KB
Cached:     128292 KB
VmPeak:      66084 KB
VmSize:      65064 KB
VmLck:           0 KB
VmPin:           0 KB
VmHWM:       14288 KB
VmRSS:       14288 KB
VmData:      12196 KB
VmStk:         136 KB
VmExe:           8 KB
VmLib:       20916 KB
VmPTE:          42 KB
VmSwap:          0 KB

Threads Information
Threads: 2
PID = 21538 TID = 21538
21538 21774 

Maps Information
40000000 40002000 r-xp /opt/usr/apps/com.sec.starfish.p03/bin/p03
4000b000 40028000 r-xp /lib/ld-2.13.so
40031000 40035000 r-xp /usr/lib/libsys-assert.so
4003e000 400fa000 r-xp /opt/usr/apps/com.sec.starfish.p03/lib/libescargot.so
40120000 40180000 r-xp /opt/usr/apps/com.sec.starfish.p03/lib/libStarFish.so
40183000 40197000 r-xp /lib/libpthread-2.13.so
401a2000 401a3000 r-xp /lib/libBrokenLocale-2.13.so
401ac000 401ae000 r-xp /usr/lib/libcapi-appfw-app-common.so.0.3.2.5
401b7000 401bc000 r-xp /usr/lib/libcapi-appfw-widget-application.so.1.1
401c4000 401c6000 r-xp /usr/lib/libdlog.so.0.0.0
401ce000 40308000 r-xp /usr/lib/libelementary.so.1.7.99
4031e000 403ed000 r-xp /usr/lib/libevas.so.1.7.99
40411000 40439000 r-xp /usr/lib/libfontconfig.so.1.8.0
4043a000 404f9000 r-xp /opt/usr/apps/com.sec.starfish.p03/lib/libstdc++.so.6
4050c000 40627000 r-xp /lib/libc-2.13.so
40635000 4063d000 r-xp /lib/libgcc_s-4.6.so.1
4063e000 40649000 r-xp /lib/libunwind.so.8.0.1
40676000 40678000 r-xp /lib/libdl-2.13.so
40681000 406ea000 r-xp /lib/libm-2.13.so
406f3000 4070a000 r-xp /usr/lib/libecore.so.1.7.99
40721000 40727000 r-xp /lib/librt-2.13.so
40730000 40750000 r-xp /usr/lib/libefl-extension.so.0.1.0
40751000 407a7000 r-xp /usr/lib/libfreetype.so.6.11.3
407b3000 407f7000 r-xp /usr/lib/libcurl.so.4.3.0
40800000 4080d000 r-xp /usr/lib/libaul.so.0.1.0
40817000 40839000 r-xp /usr/lib/libpkgmgr-info.so.0.0.17
40842000 40847000 r-xp /usr/lib/libcapi-appfw-app-control.so.0.3.2.5
4084f000 40858000 r-xp /usr/lib/libvconf.so.0.2.45
40860000 40869000 r-xp /usr/lib/libwidget_provider_app.so.1.0.0
40871000 40874000 r-xp /usr/lib/libbundle.so.0.1.22
4087c000 40888000 r-xp /usr/lib/libwidget.so.1.0.0
40890000 40960000 r-xp /usr/lib/libglib-2.0.so.0.3200.3
40961000 40966000 r-xp /usr/lib/libcapi-system-info.so.0.2.0
4096e000 40974000 r-xp /usr/lib/libcapi-base-common.so.0.1.8
4097d000 40996000 r-xp /usr/lib/libeet.so.1.7.99
409a7000 409e8000 r-xp /usr/lib/libeina.so.1.7.99
409f1000 40a13000 r-xp /usr/lib/libecore_evas.so.1.7.99
40a1c000 40a21000 r-xp /usr/lib/libecore_file.so.1.7.99
40a29000 40a3a000 r-xp /usr/lib/libecore_input.so.1.7.99
40a42000 40a9f000 r-xp /usr/lib/libedje.so.1.7.99
40aa9000 40af9000 r-xp /usr/lib/libecore_x.so.1.7.99
40afb000 40b04000 r-xp /usr/lib/libedbus.so.1.7.99
40b0c000 40b36000 r-xp /usr/lib/libdbus-1.so.3.8.12
40b3f000 40b59000 r-xp /usr/lib/libecore_con.so.1.7.99
40b62000 40b75000 r-xp /usr/lib/libfribidi.so.0.3.1
40b7d000 40bba000 r-xp /usr/lib/libharfbuzz.so.0.940.0
40bc3000 40c19000 r-xp /usr/lib/libpixman-1.so.0.28.2
40c26000 40c3c000 r-xp /lib/libexpat.so.1.5.2
40c46000 40cfe000 r-xp /usr/lib/libcairo.so.2.11200.14
40d09000 40dea000 r-xp /usr/lib/libX11.so.6.3.0
40df5000 40dfe000 r-xp /usr/lib/libXi.so.6.1.0
40e06000 40e1c000 r-xp /lib/libz.so.1.2.5
40e24000 40e3c000 r-xp /usr/lib/libpng12.so.0.50.0
40e44000 40e4d000 r-xp /usr/lib/libcares.so.2.1.0
40e56000 40e84000 r-xp /usr/lib/libidn.so.11.5.44
40e8c000 40ed9000 r-xp /usr/lib/libssl.so.1.0.0
40ee5000 4108d000 r-xp /usr/lib/libcrypto.so.1.0.0
410a6000 410bd000 r-xp /usr/lib/libdbus-glib-1.so.2.2.2
410c5000 410ca000 r-xp /usr/lib/libxdgmime.so.1.1.0
410d2000 4110e000 r-xp /usr/lib/libsystemd.so.0.4.0
41117000 4111a000 r-xp /usr/lib/libproc-stat.so.0.2.86
41122000 4113c000 r-xp /usr/lib/libpkgmgr_parser.so.0.1.0
41144000 411b9000 r-xp /usr/lib/libsqlite3.so.0.8.6
411c3000 411c9000 r-xp /usr/lib/libappsvc.so.0.1.0
411d2000 411d5000 r-xp /usr/lib/libsmack.so.1.0.0
411dd000 411f2000 r-xp /usr/lib/libcom-core.so.0.0.1
411fa000 41210000 r-xp /usr/lib/libwidget_service.so.1.0.0
41219000 4122c000 r-xp /usr/lib/libwidget_provider.so.1.0.0
41234000 41236000 r-xp /usr/lib/libiniparser.so.0
4123f000 4130b000 r-xp /usr/lib/libxml2.so.2.7.8
41318000 4133b000 r-xp /usr/lib/libjpeg.so.8.0.2
41353000 41358000 r-xp /usr/lib/libecore_fb.so.1.7.99
41361000 41365000 r-xp /usr/lib/libecore_ipc.so.1.7.99
4136e000 41371000 r-xp /usr/lib/libecore_input_evas.so.1.7.99
41379000 41380000 r-xp /usr/lib/libembryo.so.1.7.99
41388000 4139f000 r-xp /usr/lib/liblua-5.1.so
413a8000 413ae000 r-xp /usr/lib/libecore_imf.so.1.7.99
413b6000 413b7000 r-xp /usr/lib/libecore_imf_evas.so.1.7.99
413bf000 413c6000 r-xp /usr/lib/libXcursor.so.1.0.2
413ce000 413d0000 r-xp /usr/lib/libXdamage.so.1.1.0
413d8000 413da000 r-xp /usr/lib/libXcomposite.so.1.0.0
413e2000 413e4000 r-xp /usr/lib/libXgesture.so.7.0.0
413ed000 413f0000 r-xp /usr/lib/libXfixes.so.3.1.0
413f8000 413f9000 r-xp /usr/lib/libXinerama.so.1.0.0
41402000 41408000 r-xp /usr/lib/libXrandr.so.2.2.0
41410000 41416000 r-xp /usr/lib/libXrender.so.1.3.0
4141e000 41421000 r-xp /usr/lib/libXtst.so.6.1.0
4142a000 41434000 r-xp /usr/lib/libXext.so.6.4.0
4143e000 4144c000 r-xp /usr/lib/libGLESv2.so.2.0
41455000 41456000 r-xp /usr/lib/libxcb-shm.so.0.0.0
4145f000 41465000 r-xp /usr/lib/libxcb-render.so.0.0.0
4146d000 41480000 r-xp /usr/lib/libxcb.so.1.1.0
41489000 4148c000 r-xp /usr/lib/libEGL.so.1.4
41495000 41568000 r-xp /usr/lib/libgio-2.0.so.0.3200.3
41573000 415a7000 r-xp /usr/lib/libgobject-2.0.so.0.3200.3
415b0000 415c8000 r-xp /usr/lib/liblzma.so.5.0.3
415d0000 4164c000 r-xp /usr/lib/libgcrypt.so.20.0.3
41658000 41668000 r-xp /lib/libresolv-2.13.so
4166d000 41670000 r-xp /usr/lib/libSLP-db-util.so.0.1.0
41678000 4167a000 r-xp /usr/lib/journal/libjournal.so.0.1.0
41682000 41766000 r-xp /usr/lib/libicuuc.so.51.1
4177b000 41783000 r-xp /usr/lib/libdrm.so.2.4.0
4178b000 4178d000 r-xp /usr/lib/libdri2.so.0.0.0
41796000 4179d000 r-xp /usr/lib/libtbm.so.1.0.0
417a5000 417a7000 r-xp /usr/lib/libXau.so.6.0.0
417af000 417b0000 r-xp /usr/lib/libgthread-2.0.so.0.3200.3
417b9000 417be000 r-xp /usr/lib/libffi.so.5.0.10
417c6000 417c8000 r-xp /usr/lib/libgmodule-2.0.so.0.3200.3
417d0000 417db000 r-xp /usr/lib/libgpg-error.so.0.15.0
42e6f000 42f39000 r-xp /usr/lib/libCOREGL.so.4.0
432be000 43abd000 rw-p [stack:21774]
43abd000 43ac1000 r-xp /usr/lib/bufmgr/libtbm_exynos4412.so.0.0.0
43ac9000 43acc000 r-xp /usr/lib/evas/modules/engines/buffer/linux-gnueabi-armv7l-1.7.99/module.so
43ad4000 43adf000 r-xp /usr/lib/evas/modules/engines/software_generic/linux-gnueabi-armv7l-1.7.99/module.so
bedf3000 bee14000 rw-p [stack]
End of Maps Information

Callstack Information (PID:21538)
Call Stack Count: 1
 0: escargot::hex2char(char, char) + 0x3d (0x4009ce30) [/opt/usr/apps/com.sec.starfish.p03/lib/libescargot.so] + 0x5ee30
End of Call Stack

Package Information
Package Name: com.sec.starfish.p03
Package ID : com.sec.starfish.p03
Version: 1.0.0
Package Type: rpm
App Name: p03
App ID: com.sec.starfish.p03
Type: capp
Categories: 

Latest Debug Message Information
--------- beginning of /dev/log_main
     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.190+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.190+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.190+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(112) ev->cur.canvas.y(305)
02-03 16:47:17.190+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.190+0900 E/EFL     (  729): evas_main<729> evas_events.c:1258 evas_event_feed_mouse_up() ButtonEvent:up time=58291922 button=1 downs=0
02-03 16:47:17.190+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _up_cb(4112) > ad->scroller_freeze_ref_cnt [0]
02-03 16:47:17.190+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _up_cb(4116) > scroller block status - ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL
02-03 16:47:17.195+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:2277 _elm_scroll_post_event_up() [DDO] lock set false. : obj(4a0cda90), type(elm_scroller)
02-03 16:47:17.195+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:17.195+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:17.195+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:17.200+0900 D/ADD_VIEWER(  729): add-viewer.c: preview_up_cb(1033) > [SECURE_LOG] Return
02-03 16:47:17.210+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.230+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.245+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.260+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.275+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.290+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.315+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.315+0900 I/efl-extension(  729): efl_extension_circle_object_scroller.c: _eext_circle_object_scroller_scroll_animatioin_stop_cb(501) > [0x4a0cda90 : elm_scroller] CurrentPage(10)
02-03 16:47:17.835+0900 D/EFL     (  729): ecore_x<729> ecore_x_events.c:563 _ecore_x_event_handle_button_press() ButtonEvent:press time=58292579 button=1
02-03 16:47:17.835+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4438) > sd->block_dismiss_event [1]
02-03 16:47:17.835+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.835+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(336) ev->cur.canvas.y(168)
02-03 16:47:17.835+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.835+0900 E/EFL     (  729): evas_main<729> evas_events.c:994 evas_event_feed_mouse_down() ButtonEvent:down time=58292579 button=1 downs=1
02-03 16:47:17.845+0900 D/W_HOME  (  729): layout.c: _down_cb(281) > Mouse is down on the layout
02-03 16:47:17.845+0900 D/W_HOME  (  729): layout.c: _down_cb(281) > Mouse is down on the layout
02-03 16:47:17.845+0900 D/W_HOME  (  729): layout.c: _down_cb(281) > Mouse is down on the layout
02-03 16:47:17.845+0900 D/W_HOME  (  729): layout.c: _gesture_down_cb(491) > Mouse is down on the gesture layer:168
02-03 16:47:17.850+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.850+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.850+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(326) ev->cur.canvas.y(170)
02-03 16:47:17.850+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.850+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.855+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.855+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(312) ev->cur.canvas.y(172)
02-03 16:47:17.855+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.865+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(299) ev->cur.canvas.y(178)
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:4248 _elm_scroll_mouse_move_event_cb() [DDO] animator
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3352 _elm_scroll_post_event_move() [DDO] obj(4a0cda90), type(elm_scroller)
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3353 _elm_scroll_post_event_move() [DDO] hold_parent(0)
02-03 16:47:17.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3381 _elm_scroll_post_event_move() [DDO] elm_widget_drag_lock_x_set : obj(4a0cda90), type(elm_scroller)
02-03 16:47:17.870+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3616 _elm_scroll_hold_animator() [DDO] obj(4a0cda90), locked_x(0)
02-03 16:47:17.870+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3842 _elm_scroll_hold_animator() [DDO] obj(4a0cda90)
02-03 16:47:17.875+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.890+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.890+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.890+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(283) ev->cur.canvas.y(185)
02-03 16:47:17.890+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.890+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.890+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.890+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(261) ev->cur.canvas.y(193)
02-03 16:47:17.890+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.895+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3616 _elm_scroll_hold_animator() [DDO] obj(4a0cda90), locked_x(0)
02-03 16:47:17.895+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3842 _elm_scroll_hold_animator() [DDO] obj(4a0cda90)
02-03 16:47:17.900+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.915+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(241) ev->cur.canvas.y(204)
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.915+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(228) ev->cur.canvas.y(216)
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3616 _elm_scroll_hold_animator() [DDO] obj(4a0cda90), locked_x(0)
02-03 16:47:17.915+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3842 _elm_scroll_hold_animator() [DDO] obj(4a0cda90)
02-03 16:47:17.920+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.935+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(209) ev->cur.canvas.y(227)
02-03 16:47:17.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.935+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(182) ev->cur.canvas.y(245)
02-03 16:47:17.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.940+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3616 _elm_scroll_hold_animator() [DDO] obj(4a0cda90), locked_x(0)
02-03 16:47:17.940+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3842 _elm_scroll_hold_animator() [DDO] obj(4a0cda90)
02-03 16:47:17.945+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.945+0900 D/ADD_VIEWER(  729): add-viewer.c: _change_focus(728) > [SECURE_LOG] pre_focus:0x4a05b3a8 focus_widget:0x48cb74d0
02-03 16:47:17.970+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(153) ev->cur.canvas.y(266)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.970+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(130) ev->cur.canvas.y(280)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.970+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(111) ev->cur.canvas.y(291)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3616 _elm_scroll_hold_animator() [DDO] obj(4a0cda90), locked_x(0)
02-03 16:47:17.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3842 _elm_scroll_hold_animator() [DDO] obj(4a0cda90)
02-03 16:47:17.975+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:17.985+0900 D/PROCESSMGR(  366): e_mod_processmgr.c: _e_mod_processmgr_anr_ping(513) > [PROCESSMGR] ev_win=0x60002f  register trigger_timer!  pointed_win=0x60005b 
02-03 16:47:18.000+0900 D/EFL     (  729): ecore_x<729> ecore_x_events.c:722 _ecore_x_event_handle_button_release() ButtonEvent:release time=58292729 button=1
02-03 16:47:18.000+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.000+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.000+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(101) ev->cur.canvas.y(307)
02-03 16:47:18.000+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.000+0900 E/EFL     (  729): evas_main<729> evas_events.c:1258 evas_event_feed_mouse_up() ButtonEvent:up time=58292729 button=1 downs=0
02-03 16:47:18.005+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _up_cb(4112) > ad->scroller_freeze_ref_cnt [0]
02-03 16:47:18.005+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _up_cb(4116) > scroller block status - ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL
02-03 16:47:18.010+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:2277 _elm_scroll_post_event_up() [DDO] lock set false. : obj(4a0cda90), type(elm_scroller)
02-03 16:47:18.010+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:18.010+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:18.010+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:18.015+0900 D/ADD_VIEWER(  729): add-viewer.c: preview_up_cb(1033) > [SECURE_LOG] Return
02-03 16:47:18.020+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:18.040+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:18.060+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:18.075+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:18.085+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:18.135+0900 D/ADD_VIEWER(  729): add-viewer.c: _widget_scroll_cb(785) > [SECURE_LOG] ========== Widget Scroll CB is called (0x4a0cda90)
02-03 16:47:18.140+0900 I/efl-extension(  729): efl_extension_circle_object_scroller.c: _eext_circle_object_scroller_scroll_animatioin_stop_cb(501) > [0x4a0cda90 : elm_scroller] CurrentPage(11)
02-03 16:47:18.790+0900 D/EFL     (  729): ecore_x<729> ecore_x_events.c:563 _ecore_x_event_handle_button_press() ButtonEvent:press time=58293533 button=1
02-03 16:47:18.790+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4438) > sd->block_dismiss_event [1]
02-03 16:47:18.790+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.790+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(224) ev->cur.canvas.y(216)
02-03 16:47:18.790+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.790+0900 E/EFL     (  729): evas_main<729> evas_events.c:994 evas_event_feed_mouse_down() ButtonEvent:down time=58293533 button=1 downs=1
02-03 16:47:18.800+0900 D/W_HOME  (  729): layout.c: _down_cb(281) > Mouse is down on the layout
02-03 16:47:18.800+0900 D/W_HOME  (  729): layout.c: _down_cb(281) > Mouse is down on the layout
02-03 16:47:18.800+0900 D/W_HOME  (  729): layout.c: _down_cb(281) > Mouse is down on the layout
02-03 16:47:18.800+0900 D/W_HOME  (  729): layout.c: _gesture_down_cb(491) > Mouse is down on the gesture layer:216
02-03 16:47:18.805+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.805+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.805+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(220) ev->cur.canvas.y(214)
02-03 16:47:18.805+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.805+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.805+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.805+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(215) ev->cur.canvas.y(210)
02-03 16:47:18.805+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.810+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.810+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.810+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(211) ev->cur.canvas.y(208)
02-03 16:47:18.810+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.825+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.825+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.825+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(210) ev->cur.canvas.y(206)
02-03 16:47:18.825+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.835+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.835+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.835+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(210) ev->cur.canvas.y(205)
02-03 16:47:18.835+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.845+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.845+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.845+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(211) ev->cur.canvas.y(205)
02-03 16:47:18.845+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.855+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.855+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.855+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(217) ev->cur.canvas.y(205)
02-03 16:47:18.855+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.865+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _move_cb(4433) > touch_move_skipped [1]
02-03 16:47:18.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3964 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), block(1)
02-03 16:47:18.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3978 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), ev->cur.canvas.x(221) ev->cur.canvas.y(203)
02-03 16:47:18.865+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:3979 _elm_scroll_mouse_move_event_cb() [DDO] obj(4a0cda90), hold(0) freeze(0)
02-03 16:47:18.880+0900 D/PROCESSMGR(  366): e_mod_processmgr.c: _e_mod_processmgr_anr_ping(513) > [PROCESSMGR] ev_win=0x60002f  register trigger_timer!  pointed_win=0x60005b 
02-03 16:47:18.880+0900 D/EFL     (  729): ecore_x<729> ecore_x_events.c:722 _ecore_x_event_handle_button_release() ButtonEvent:release time=58293624 button=1
02-03 16:47:18.880+0900 E/EFL     (  729): evas_main<729> evas_events.c:1258 evas_event_feed_mouse_up() ButtonEvent:up time=58293624 button=1 downs=0
02-03 16:47:18.880+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _up_cb(4112) > ad->scroller_freeze_ref_cnt [0]
02-03 16:47:18.880+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _up_cb(4116) > scroller block status - ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL
02-03 16:47:18.890+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:18.890+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:18.890+0900 D/W_HOME  (  729): layout.c: _up_cb(344) > Mouse is up on the layout
02-03 16:47:18.890+0900 D/ADD_VIEWER(  729): add-viewer.c: preview_up_cb(1033) > [SECURE_LOG] Return
02-03 16:47:18.900+0900 D/W_HOME  (  729): edit.c: _widget_selected_cb(2283) > Selected: com.sec.starfish.p03
02-03 16:47:18.905+0900 D/W_HOME  (  729): dbox.c: widget_create(366) > Mark added: com.sec.starfish.p03
02-03 16:47:18.905+0900 D/W_HOME  (  729): log_manager.c: log_manager_insert_log(64) > [SECURE_LOG] appid[com.samsung.w-home], feature[1007], extra[com.sec.starfish.p03]
02-03 16:47:18.905+0900 I/samsung-log-manager(  729): SLogMgr_SamsungLogManager.cpp: samsung_log_manager_insert(542) > [SECURE_LOG] [(542)]: Insert: User consent not taken for using samsung log service
02-03 16:47:18.905+0900 E/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_resize(5435) > [SECURE_LOG] Failed to get box size
02-03 16:47:18.905+0900 E/WIDGET_VIEWER(  729): widget.c: widget_viewer_is_created_by_user(3452) > [SECURE_LOG] Handler is invalid
02-03 16:47:18.905+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_resize(5470) > [SECURE_LOG] Request size change: 360x360 [0x4]
02-03 16:47:18.905+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_resize(5485) > [SECURE_LOG] Create new handle: 360x360, (com.sec.starfish.p03, (null)), user,created/default
02-03 16:47:18.910+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_resize(5506) > [SECURE_LOG] Added handle: 0x4a7bf3d0 (0x4a729308)
02-03 16:47:18.910+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_overlay_loading(5287) > [SECURE_LOG] Loading overlay is disabled
02-03 16:47:18.930+0900 D/W_HOME  (  729): scroller.c: _elm_box_unpack(375) > No page to unpack
02-03 16:47:18.930+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(0), origin_y(0)
02-03 16:47:18.930+0900 D/W_HOME  (  729): scroller.c: _elm_box_pack_end(267) > pack_end a page(0x4a74fa80) into the scroller(0x45d407f0), origin_reverse(0)
02-03 16:47:18.935+0900 D/W_HOME  (  729): page.c: _move_cb(67) > Page(0x4a74fa80) is appended into the scroller (360:0:0:0)
02-03 16:47:18.935+0900 D/W_HOME  (  729): scroller.c: scroller_pack_page_inners(513) > Pack all page_inners
02-03 16:47:18.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(0), origin_y(0)
02-03 16:47:18.935+0900 D/W_HOME  (  729): scroller.c: _elm_box_unpack(388) > unpack a page(0x47be4530) from the scroller(0x45d407f0), origin_reverse(0)
02-03 16:47:18.935+0900 D/W_HOME  (  729): scroller.c: scroller_pack_page_inners(513) > Pack all page_inners
02-03 16:47:18.935+0900 D/W_HOME  (  729): scroller.c: _elm_box_unpack(375) > No page to unpack
02-03 16:47:18.935+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(0), origin_y(0)
02-03 16:47:18.940+0900 D/W_HOME  (  729): scroller.c: _elm_box_pack_end(267) > pack_end a page(0x47be4530) into the scroller(0x45d407f0), origin_reverse(0)
02-03 16:47:18.940+0900 D/W_HOME  (  729): scroller.c: scroller_pack_page_inners(513) > Pack all page_inners
02-03 16:47:18.940+0900 W/W_HOME  (  729): db.c: db_read_list(807) > push all the pages into the DB
02-03 16:47:18.940+0900 D/W_HOME  (  729): db.c: db_remove_all_item(604) > Remove all the item
02-03 16:47:18.945+0900 D/W_HOME  (  729): db.c: db_remove_all_item(623) > [SECURE_LOG] Set db/private/com.samsung.w-home/logging as ';'
02-03 16:47:18.945+0900 D/W_HOME  (  729): db.c: db_insert_item(452) > Insert the item[com.sec.starfish:0]
02-03 16:47:18.950+0900 D/W_HOME  (  729): db.c: db_insert_item(452) > Insert the item[com.sec.starfish.f06:1]
02-03 16:47:18.950+0900 D/W_HOME  (  729): db.c: db_insert_item(452) > Insert the item[com.sec.starfish.p03:2]
02-03 16:47:18.950+0900 D/W_HOME  (  729): db.c: db_read_list(826) > [SECURE_LOG] Logging : [com.sec.starfish;com.sec.starfish.f06;com.sec.starfish.p03]
02-03 16:47:18.955+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(1), origin_y(1)
02-03 16:47:18.955+0900 D/W_HOME  (  729): scroller.c: _elm_box_unpack(383) > unpack a page(0x4ab01420) from the scroller(0x45d407f0), origin_reverse(1)
02-03 16:47:18.955+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish.f06 has no changes
02-03 16:47:18.960+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.samsung.w-watch-chrono has no changes
02-03 16:47:18.960+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish.f06 has no changes
02-03 16:47:18.960+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-1080] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:18.960+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(1), origin_y(1)
02-03 16:47:18.960+0900 D/W_HOME  (  729): scroller.c: _elm_box_pack_after(330) > pack_after a page(0x4ab01420) after a page(0x4a77eb68) into the scroller(0x45d407f0), origin_reverse(1)
02-03 16:47:18.960+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish has no changes
02-03 16:47:18.960+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: update_visibility(4040) > [SECURE_LOG] Delayed resume (0.400000)
02-03 16:47:18.965+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.samsung.w-watch-chrono has no changes
02-03 16:47:18.965+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish.f06 has no changes
02-03 16:47:18.965+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-1440] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:18.965+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish has no changes
02-03 16:47:18.965+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(1), origin_y(1)
02-03 16:47:18.965+0900 D/W_HOME  (  729): scroller.c: _elm_box_unpack(383) > unpack a page(0x4ab00820) from the scroller(0x45d407f0), origin_reverse(1)
02-03 16:47:18.965+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.samsung.w-watch-chrono has no changes
02-03 16:47:18.965+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish has no changes
02-03 16:47:18.965+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-1080] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:18.965+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(1), origin_y(1)
02-03 16:47:18.965+0900 D/W_HOME  (  729): scroller.c: _elm_box_pack_after(330) > pack_after a page(0x4ab00820) after a page(0x4ab01420) into the scroller(0x45d407f0), origin_reverse(1)
02-03 16:47:18.970+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: update_visibility(4040) > [SECURE_LOG] Delayed resume (0.400000)
02-03 16:47:18.970+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.samsung.w-watch-chrono has no changes
02-03 16:47:18.970+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish has no changes
02-03 16:47:18.970+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-1440] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:18.970+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish.f06 has no changes
02-03 16:47:18.970+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(0), origin_y(0)
02-03 16:47:18.970+0900 D/W_HOME  (  729): scroller.c: _elm_box_unpack(388) > unpack a page(0x4a74fa80) from the scroller(0x45d407f0), origin_reverse(0)
02-03 16:47:18.975+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(1), origin_y(1)
02-03 16:47:18.975+0900 D/W_HOME  (  729): scroller.c: _elm_box_pack_after(330) > pack_after a page(0x4a74fa80) after a page(0x4ab00820) into the scroller(0x45d407f0), origin_reverse(1)
02-03 16:47:18.975+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.samsung.w-watch-chrono has no changes
02-03 16:47:18.975+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish has no changes
02-03 16:47:18.975+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish.f06 has no changes
02-03 16:47:18.975+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-1800] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:18.975+0900 W/W_HOME  (  729): scroller.c: _get_index_in_list(1902) > page:0x4a74fa80 idx:4 total6 exist:1
02-03 16:47:18.975+0900 D/W_HOME  (  729): wms.c: wms_change_favorite_order(92) > Favorite Backup
02-03 16:47:18.980+0900 E/WMS     (  528): wms_event_handler.c: _wms_event_handler_cb_vconf_favorites_order(6419) > ConnectionType[0] is not BT, ignore this callback.
02-03 16:47:19.005+0900 D/W_HOME  (  729): scroller.c: _change_favorites_order_cb(1413) > Change favorites order
02-03 16:47:19.005+0900 D/W_HOME  (  729): scroller.c: _change_favorites_order_cb(1427) > Change favorites order vconf:[3]
02-03 16:47:19.005+0900 D/W_HOME  (  729): wms.c: wms_change_favorite_order(113) > done
02-03 16:47:19.005+0900 D/DATA_PROVIDER_MASTER(  704): server.c: client_activate_package(6306) > [SECURE_LOG] pid[729] pkgname[com.sec.starfish.p03]
02-03 16:47:19.010+0900 D/W_HOME  (  729): page.c: page_access_highlight(824) > focus set to 0x47e59b50
02-03 16:47:19.010+0900 E/EFL     (  729): elementary<729> elm_interface_scrollable.c:1270 _elm_scroll_origin_reverse_set() [DDO] obj(45d407f0), origin_x(0), origin_y(0)
02-03 16:47:19.010+0900 D/W_HOME  (  729): scroller.c: _page_region_show(2468) > Page show now : 4(0x4a74fa80)
02-03 16:47:19.015+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.samsung.w-watch-chrono has no changes
02-03 16:47:19.015+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish has no changes
02-03 16:47:19.015+0900 D/WIDGET_VIEWER(  729): widget.c: widget_viewer_set_visibility(3859) > [SECURE_LOG] com.sec.starfish.f06 has no changes
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-1440] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: elm_rpanel_scroll_x_pos_set(6834) > scroll_x [1440]
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: elm_rpanel_scroll_x_pos_set(6848) > Decide direction => none
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _elm_rpanel_smart_move(1504) > smart move, in viewport value [0], x [-360] y [0], ow [360] oh [360] sd->scroll_x [1440]
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: elm_rpanel_noti_detail_is_showing(4631) > ret [0]
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: elm_rpanel_scroll_x_pos_set(6885) > panel_clear_y [-100]
02-03 16:47:19.015+0900 I/ELM_RPANEL(  729): elm-rpanel.c: _panel_swap_effect(2829) > tobj_item_01 is null
02-03 16:47:19.015+0900 I/ELM_RPANEL(  729): elm-rpanel.c: _panel_swap_effect(2884) > tobj_item_02 is null
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _panel_swap_effect(2940) > noti panel count [1], tobj_layout_01 0x0, tobj_layout_02 0x0
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _panel_swap_effect(2943) > Need to cancel swap effect.
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _panel_swap_effect(2961) > Call swap effect done manually.
02-03 16:47:19.015+0900 D/ELM_RPANEL(  729): elm-rpanel.c: _panel_swap_effect_done(2703) > Check noti panels visibility.
02-03 16:47:19.015+0900 E/W_HOME  (  729): index_imp_custom.c: _bring_in_page(68) > Cannot find a page(0x4a74fa80)
02-03 16:47:19.035+0900 D/DATA_PROVIDER_MASTER(  704): server.c: client_new(1411) > [SECURE_LOG] pid[729] period[58293.652344] pkgname[com.sec.starfish.p03] content[] cluster[user,created] category[default] period[-1.000000]
02-03 16:47:19.055+0900 D/W_HOME  (  729): index_imp_custom.c: _update(403) > Index(0x45d5c9c8) is clear
02-03 16:47:19.070+0900 D/W_HOME  (  729): util_time.c: __util_time_formatted_time_get(473) > time(1454485639) formatted(4:47)
02-03 16:47:19.075+0900 D/DATA_PROVIDER_MASTER(  704): util.c: util_free_space(150) > [SECURE_LOG] Available size: 2016403456, f_bsize: 4096, f_bavail: 0
02-03 16:47:19.075+0900 D/DATA_PROVIDER_MASTER(  704): slave_life.c: slave_load_instance(1712) > [SECURE_LOG] Instance: (21538)2
02-03 16:47:19.075+0900 D/WIDGET_PROVIDER_APP(21538): client.c: method_new(479) > [SECURE_LOG] Create: pkgname[com.sec.starfish.p03], id[file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png], content[], timeout[60], has_script[0], period[0.000000], cluster[user,created], category[default], skip[1], abi[app], size: 360x360, ori: 0
02-03 16:47:19.075+0900 D/W_HOME  (  729): page_indicator.c: page_indicator_item_at(246) > index:-1
02-03 16:47:19.075+0900 E/W_HOME  (  729): page_indicator.c: page_indicator_item_at(248) > (index < 0 || index >= PAGE_INDICATOR_MAX) -> page_indicator_item_at() return
02-03 16:47:19.080+0900 D/W_HOME  (  729): scroller.c: _index_update_cb(1215) > Index is updated
02-03 16:47:19.085+0900 D/WIDGET_PROVIDER(21538): widget_provider_buffer.c: widget_provider_buffer_create(2012) > [SECURE_LOG] acquire_buffer: [WIDGET] file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png, handler: 0x40881f35
02-03 16:47:19.085+0900 D/WIDGET  (21538): binder.c: binder_ecore_evas_new(537) > [SECURE_LOG] Preferred engine: (null) (opengl or opengl_x11)
02-03 16:47:19.085+0900 D/WIDGET  (21538): binder.c: binder_ecore_evas_new(549) > [SECURE_LOG] Preferred engine: (null) (opengl)
02-03 16:47:19.090+0900 D/WIDGET_PROVIDER(21538): widget_provider_buffer.c: widget_provider_buffer_acquire(2084) > [SECURE_LOG] acquire_buffer: [WIDGET] file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png, 1x1, size: 4, handler: 0x40881f35
02-03 16:47:19.090+0900 D/COM_CORE(  704): com-core_thread.c: tcb_create(564) > [SECURE_LOG] [55] New TCB created: R(91), W(92)
02-03 16:47:19.090+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(350) > [SECURE_LOG] Thread is created for 55 (server: 4)
02-03 16:47:19.090+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: buffer_handler_resize(1060) > [SECURE_LOG] Buffer size is updated[1x1]
02-03 16:47:19.090+0900 D/WIDGET_SERVICE(  704): widget_service.c: widget_service_create_resource_lock(3534) > [SECURE_LOG] Create a file named [/opt/usr/share/live_magazine/.widget.lck/.27264042]
02-03 16:47:19.090+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: load_pixmap_buffer(552) > [SECURE_LOG] Loaded pixmap(info->id): pixmap://27264042:4
02-03 16:47:19.095+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(655) > [SECURE_LOG] Recv'd size: 64 (header: 64) pid: 704, fd: -1
02-03 16:47:19.095+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(701) > [SECURE_LOG] Recv'd 24 bytes (pid: 704), fd: -1
02-03 16:47:19.095+0900 E/COM_CORE(  704): com-core_thread.c: client_cb(397) > [SECURE_LOG] Available data: 0
02-03 16:47:19.095+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(433) > [SECURE_LOG] Client CB is terminated (55)
02-03 16:47:19.095+0900 D/COM_CORE(  704): com-core_thread.c: com_core_thread_recv_with_fd(1103) > [SECURE_LOG] Disconnected
02-03 16:47:19.095+0900 D/COM_CORE(  704): com-core_packet.c: service_cb(460) > [SECURE_LOG] ZERO bytes receives(21538)
02-03 16:47:19.095+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(498) > [SECURE_LOG] Service callback returns -104 < 0
02-03 16:47:19.095+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(505) > [SECURE_LOG] Disconnecting
02-03 16:47:19.095+0900 D/COM_CORE(  704): com-core_packet.c: client_disconnected_cb(361) > [SECURE_LOG] Clean up all requests and a receive context for handle(55) for pid(-1)
02-03 16:47:19.095+0900 E/COM_CORE(  704): com-core_thread.c: terminate_thread(192) > [SECURE_LOG] Thread returns: -104
02-03 16:47:19.095+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(718) > [SECURE_LOG] Close connection: 36 (recv_fd: -1)
02-03 16:47:19.095+0900 D/WIDGET_PROVIDER(21538): fb.c: fb_create(650) > [SECURE_LOG] PIXMAP: 27264042
02-03 16:47:19.095+0900 D/WIDGET_PROVIDER(21538): widget_provider_buffer.c: send_acquire_request(148) > [SECURE_LOG] type: 0x0, name: 26_57926.163053, pkgname[com.sec.starfish.p03], id[file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png], w[1], h[1], size[4], buffer_id[pixmap://27264042:4], fb[0x42e38880]
02-03 16:47:19.095+0900 D/WIDGET_PROVIDER(21538): fb.c: create_gem(495) > [SECURE_LOG] dri2_buffer: 0x42e3d568, name: 0xb, 1x1 (1x1), pitch: 8, buf_count: 1, gem: 0x42d7a9e8
02-03 16:47:19.095+0900 D/WIDGET  (21538): binder.c: alloc_fb(445) > [SECURE_LOG] HW Accelerated buffer is created 0x42e43308, (1x1)
02-03 16:47:19.095+0900 D/WIDGET_PROVIDER(21538): fb.c: fb_stride(1053) > [SECURE_LOG] Stride: 8
02-03 16:47:19.095+0900 D/WIDGET  (21538): binder.c: alloc_stride_fb(480) > [SECURE_LOG] bpp: 32, stride: 8
02-03 16:47:19.100+0900 D/WIDGET_PROVIDER(21538): fb.c: destroy_gem(510) > [SECURE_LOG] unref pixmap bo
02-03 16:47:19.100+0900 D/WIDGET  (21538): binder.c: free_fb(496) > [SECURE_LOG] HW Accelerated buffer is destroyed
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: tcb_create(564) > [SECURE_LOG] [55] New TCB created: R(91), W(92)
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(350) > [SECURE_LOG] Thread is created for 55 (server: 4)
02-03 16:47:19.100+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: destroy_pixmap(351) > [SECURE_LOG] pixmap 27264042
02-03 16:47:19.100+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(655) > [SECURE_LOG] Recv'd size: 64 (header: 64) pid: 704, fd: -1
02-03 16:47:19.100+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(701) > [SECURE_LOG] Recv'd 4 bytes (pid: 704), fd: -1
02-03 16:47:19.100+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(718) > [SECURE_LOG] Close connection: 36 (recv_fd: -1)
02-03 16:47:19.100+0900 D/WIDGET  (21538): widget.c: widget_viewer_release_buffer(609) > [SECURE_LOG] Release buffer: -1 (0)
02-03 16:47:19.100+0900 D/WIDGET  (21538): binder.c: alloc_fb(424) > [SECURE_LOG] Size of ee is updated: 360x360 (info: 0x42e43308)
02-03 16:47:19.100+0900 D/WIDGET_PROVIDER(21538): widget_provider_buffer.c: widget_provider_buffer_acquire(2084) > [SECURE_LOG] acquire_buffer: [WIDGET] file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png, 360x360, size: 4, handler: 0x40881f35
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: tcb_create(564) > [SECURE_LOG] [95] New TCB created: R(98), W(99)
02-03 16:47:19.100+0900 E/COM_CORE(  704): com-core_thread.c: client_cb(397) > [SECURE_LOG] Available data: 0
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(433) > [SECURE_LOG] Client CB is terminated (55)
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: com_core_thread_recv_with_fd(1103) > [SECURE_LOG] Disconnected
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_packet.c: service_cb(460) > [SECURE_LOG] ZERO bytes receives(564)
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(498) > [SECURE_LOG] Service callback returns -104 < 0
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(505) > [SECURE_LOG] Disconnecting
02-03 16:47:19.100+0900 D/COM_CORE(  704): com-core_packet.c: client_disconnected_cb(361) > [SECURE_LOG] Clean up all requests and a receive context for handle(55) for pid(-1)
02-03 16:47:19.105+0900 E/COM_CORE(  704): com-core_thread.c: terminate_thread(192) > [SECURE_LOG] Thread returns: -104
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(350) > [SECURE_LOG] Thread is created for 95 (server: 4)
02-03 16:47:19.105+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: buffer_handler_resize(1060) > [SECURE_LOG] Buffer size is updated[360x360]
02-03 16:47:19.105+0900 D/WIDGET_SERVICE(  704): widget_service.c: widget_service_create_resource_lock(3534) > [SECURE_LOG] Create a file named [/opt/usr/share/live_magazine/.widget.lck/.27264044]
02-03 16:47:19.105+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: load_pixmap_buffer(552) > [SECURE_LOG] Loaded pixmap(info->id): pixmap://27264044:4
02-03 16:47:19.105+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(655) > [SECURE_LOG] Recv'd size: 64 (header: 64) pid: 704, fd: -1
02-03 16:47:19.105+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(701) > [SECURE_LOG] Recv'd 24 bytes (pid: 704), fd: -1
02-03 16:47:19.105+0900 E/COM_CORE(  704): com-core_thread.c: client_cb(397) > [SECURE_LOG] Available data: 0
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(433) > [SECURE_LOG] Client CB is terminated (95)
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_thread.c: com_core_thread_recv_with_fd(1103) > [SECURE_LOG] Disconnected
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_packet.c: service_cb(460) > [SECURE_LOG] ZERO bytes receives(21538)
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(498) > [SECURE_LOG] Service callback returns -104 < 0
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(505) > [SECURE_LOG] Disconnecting
02-03 16:47:19.105+0900 D/COM_CORE(  704): com-core_packet.c: client_disconnected_cb(361) > [SECURE_LOG] Clean up all requests and a receive context for handle(95) for pid(-1)
02-03 16:47:19.105+0900 E/COM_CORE(  704): com-core_thread.c: terminate_thread(192) > [SECURE_LOG] Thread returns: -104
02-03 16:47:19.110+0900 D/COM_CORE(21538): com-core_packet.c: com_core_packet_oneshot_send(718) > [SECURE_LOG] Close connection: 36 (recv_fd: -1)
02-03 16:47:19.110+0900 D/WIDGET_PROVIDER(21538): fb.c: fb_create(650) > [SECURE_LOG] PIXMAP: 27264044
02-03 16:47:19.110+0900 D/WIDGET_PROVIDER(21538): widget_provider_buffer.c: send_acquire_request(148) > [SECURE_LOG] type: 0x0, name: 26_57926.163053, pkgname[com.sec.starfish.p03], id[file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png], w[360], h[360], size[4], buffer_id[pixmap://27264044:4], fb[0x42e577e8]
02-03 16:47:19.110+0900 D/WIDGET_PROVIDER(21538): fb.c: create_gem(495) > [SECURE_LOG] dri2_buffer: 0x42e47040, name: 0xb, 360x360 (360x360), pitch: 1440, buf_count: 1, gem: 0x42d7a9e8
02-03 16:47:19.110+0900 D/WIDGET  (21538): binder.c: alloc_fb(445) > [SECURE_LOG] HW Accelerated buffer is created 0x42e43308, (360x360)
02-03 16:47:19.110+0900 D/WIDGET_PROVIDER(21538): fb.c: fb_stride(1053) > [SECURE_LOG] Stride: 1440
02-03 16:47:19.110+0900 D/WIDGET  (21538): binder.c: alloc_stride_fb(480) > [SECURE_LOG] bpp: 32, stride: 1440
02-03 16:47:19.150+0900 D/WIDGET_VIEWER(  729): client.c: master_update_id(1651) > [SECURE_LOG] Update ID(file:///opt/usr/share/live_magazine/com.sec.starfish.p03_729_58293.652344.png) for 58293.652344
02-03 16:47:19.150+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_event_widget_size_changed(6242) > [SECURE_LOG] widget LB size is changed
02-03 16:47:19.155+0900 D/WIDGET_EVAS(  729): widget_viewer_evas.c: __widget_event_widget_size_changed(6242) > [SECURE_LOG] widget LB size is changed
02-03 16:47:19.375+0900 W/CRASH_MANAGER(21882): worker.c: worker_job(1199) > 1121538703033145448563
