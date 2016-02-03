S/W Version Information
Model: SM-R720
Tizen-Version: 2.3.1.2
Build-Number: R720XXE2BOKH
Build-Date: 2015.11.24 18:54:57

Crash Information
Process Name: clocknewstarfish
PID: 19774
Date: 2016-02-03 16:28:44+0900
Executable File Path: /opt/usr/apps/com.sec.starfish.clock/bin/clocknewstarfish
Signal: 11
      (SIGSEGV)
      si_code: 1
      address not mapped to object
      si_addr = 0x108

Register Information
r0   = 0x00000000, r1   = 0x43b07828
r2   = 0x00000001, r3   = 0x43b0c000
r4   = 0x43b07828, r5   = 0x401039f0
r6   = 0x00000001, r7   = 0x00000330
r8   = 0x40107988, r9   = 0x40107988
r10  = 0x40107988, fp   = 0x00000000
ip   = 0x40103bf0, sp   = 0xbefd5cb0
lr   = 0x4007e7ed, pc   = 0x4009ce30
cpsr = 0x00000030

Memory Information
MemTotal:   489820 KB
MemFree:     35972 KB
Buffers:     38820 KB
Cached:     148168 KB
VmPeak:      66152 KB
VmSize:      65132 KB
VmLck:           0 KB
VmPin:           0 KB
VmHWM:       14444 KB
VmRSS:       14444 KB
VmData:      12220 KB
VmStk:         136 KB
VmExe:           8 KB
VmLib:       20924 KB
VmPTE:          42 KB
VmSwap:          0 KB

Threads Information
Threads: 2
PID = 19774 TID = 19774
19774 20014 

Maps Information
40000000 40002000 r-xp /opt/usr/apps/com.sec.starfish.clock/bin/clocknewstarfish
4000b000 40028000 r-xp /lib/ld-2.13.so
40031000 40035000 r-xp /usr/lib/libsys-assert.so
4003e000 400fa000 r-xp /opt/usr/apps/com.sec.starfish.clock/lib/libescargot.so
40120000 40180000 r-xp /opt/usr/apps/com.sec.starfish.clock/lib/libStarFish.so
40183000 40197000 r-xp /lib/libpthread-2.13.so
401a2000 401a3000 r-xp /lib/libBrokenLocale-2.13.so
401ac000 401ae000 r-xp /usr/lib/libcapi-appfw-app-common.so.0.3.2.5
401b7000 401bc000 r-xp /usr/lib/libcapi-appfw-widget-application.so.1.1
401c4000 401c6000 r-xp /usr/lib/libdlog.so.0.0.0
401ce000 40308000 r-xp /usr/lib/libelementary.so.1.7.99
4031e000 403ed000 r-xp /usr/lib/libevas.so.1.7.99
40411000 40439000 r-xp /usr/lib/libfontconfig.so.1.8.0
4043a000 404f9000 r-xp /opt/usr/apps/com.sec.starfish.clock/lib/libstdc++.so.6
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
432be000 43abd000 rw-p [stack:20014]
43abd000 43ac1000 r-xp /usr/lib/bufmgr/libtbm_exynos4412.so.0.0.0
43ac9000 43acc000 r-xp /usr/lib/evas/modules/engines/buffer/linux-gnueabi-armv7l-1.7.99/module.so
43ad4000 43adf000 r-xp /usr/lib/evas/modules/engines/software_generic/linux-gnueabi-armv7l-1.7.99/module.so
43cee000 43cf0000 r-xp /usr/lib/evas/modules/loaders/png/linux-gnueabi-armv7l-1.7.99/module.so
befb6000 befd7000 rw-p [stack]
End of Maps Information

Callstack Information (PID:19774)
Call Stack Count: 1
 0: escargot::hex2char(char, char) + 0x3d (0x4009ce30) [/opt/usr/apps/com.sec.starfish.clock/lib/libescargot.so] + 0x5ee30
End of Call Stack

Package Information
Package Name: com.sec.starfish.clock
Package ID : com.sec.starfish.clock
Version: 1.0.0
Package Type: rpm
App Name: clock new starfish
App ID: com.sec.starfish.clock
Type: capp
Categories: 

Latest Debug Message Information
--------- beginning of /dev/log_main
sagePortService.cpp: CheckRemotePort(220) > Check a remote message port: [com.samsung.w-home:music-control-service-message-port]
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: Send(847) > _MessagePortIpcServer::Stop
02-03 16:28:41.780+0900 I/MESSAGE_PORT( 1118): message_port.cpp: message_port_send_message(211) > [SECURE_LOG] Send a message to the remote port (com.samsung.w-home):(music-control-service-message-port).
02-03 16:28:41.780+0900 I/MESSAGE_PORT( 1118): MessagePortProxy.cpp: SendMessage(403) > [SECURE_LOG] Send a message to : [com.samsung.w-home:music-control-service-message-port]
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: OnReadMessage(739) > _MessagePortIpcServer::OnReadMessage
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: HandleReceivedMessage(578) > _MessagePortIpcServer::HandleReceivedMessage
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: OnIpcRequestReceived(147) > MessagePort message received
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: OnSendMessage(126) > MessagePort OnSendMessage
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: SendMessage(291) > _MessagePortService::SendMessage
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: GetKey(365) > _MessagePortService::GetKey
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: GetKey(395) > [SECURE_LOG] _MessagePortService::GetKey Key:[com.samsung.w-home:music-control-service-message-port]
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: SendMessage(299) > Sends a message to a remote message port [com.samsung.w-home:music-control-service-message-port]
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: SendMessage(138) > MessagePort SendMessage
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: SendResponse(884) > _MessagePortIpcServer::SendResponse
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: Send(847) > _MessagePortIpcServer::Stop
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  729): MessagePortProxy.cpp: OnSendMessageInternal(723) > [SECURE_LOG] Message received! from App: com.samsung.w-music-player.music-control-service, to App: com.samsung.w-home, Port: music-control-service-message-port, Trusted: FALSE
02-03 16:28:41.780+0900 I/MESSAGE_PORT(  729): message_port.cpp: message_dispatcher(52) > [SECURE_LOG] A message has been received to specific local port id (1) from remote port (com.samsung.w-music-player.music-control-service):((null)).
02-03 16:28:41.780+0900 D/W_HOME  (  729): clock_shortcut.c: _music_service_messageport_cb(345) > msg_id:music-volumechanged-ind remote app id:com.samsung.w-music-player.music-control-service
02-03 16:28:41.780+0900 I/MUSIC_CONTROL_SERVICE( 1118): music-control-message.c: music_control_message_send(173) > [SECURE_LOG] [T:1118] message(music-volumechanged-ind) is sended to [com.samsung.w-home]
02-03 16:28:41.840+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:41.840+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:41.846
02-03 16:28:41.840+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.190800
02-03 16:28:41.840+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:42.035+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:42.035+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:42.41
02-03 16:28:42.035+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.191800
02-03 16:28:42.035+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:42.230+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:42.230+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:42.236
02-03 16:28:42.230+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.192800
02-03 16:28:42.230+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:42.425+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:42.430+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:42.433
02-03 16:28:42.430+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.193400
02-03 16:28:42.430+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:42.625+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:42.625+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:42.631
02-03 16:28:42.625+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.193800
02-03 16:28:42.625+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:42.825+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:42.825+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:42.829
02-03 16:28:42.825+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.194200
02-03 16:28:42.825+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:43.020+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:43.020+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:43.25
02-03 16:28:43.020+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.195000
02-03 16:28:43.020+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:43.220+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:43.220+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:43.224
02-03 16:28:43.220+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.195200
02-03 16:28:43.220+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:43.415+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:43.420+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:43.422
02-03 16:28:43.420+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.195600
02-03 16:28:43.420+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:43.615+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:43.615+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:43.620
02-03 16:28:43.615+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.196000
02-03 16:28:43.615+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:43.695+0900 D/AUL     (20199): launch.c: app_request_to_launchpad(268) > [SECURE_LOG] launch request : org.tizen.widget_viewer_sdk
02-03 16:28:43.695+0900 D/AUL     (20199): app_sock.c: __app_send_raw(251) > pid(-2) : cmd(0)
02-03 16:28:43.695+0900 W/AUL_AMD (  531): amd_request.c: __request_handler(640) > __request_handler: 0
02-03 16:28:43.695+0900 D/AUL_AMD (  531): amd_request.c: __request_handler(660) > [SECURE_LOG] alias appid: org.tizen.widget_viewer_sdk, appid: org.tizen.widget_viewer_sdk
02-03 16:28:43.710+0900 D/PKGMGR_INFO(  531): pkgmgrinfo_appinfo.c: pkgmgrinfo_appinfo_filter_foreach_appinfo(3737) > [SECURE_LOG] where = package_app_info.app_exec='/usr/bin/launch_app' and package_app_changeable_info.app_disable IN ('false','False')
02-03 16:28:43.710+0900 D/PKGMGR_INFO(  531): pkgmgrinfo_appinfo.c: pkgmgrinfo_appinfo_filter_foreach_appinfo(3743) > [SECURE_LOG] query = select DISTINCT package_app_info.*, package_app_changeable_info.*, package_app_localized_info.app_locale, package_app_localized_info.app_label, package_app_localized_info.app_icon from package_app_info LEFT OUTER JOIN package_app_localized_info ON package_app_info.app_id=package_app_localized_info.app_id and package_app_localized_info.app_locale IN ('No Locale', 'ko-kr') LEFT OUTER JOIN package_app_app_svc ON package_app_info.app_id=package_app_app_svc.app_id LEFT OUTER JOIN package_app_app_category ON package_app_info.app_id=package_app_app_category.app_id LEFT OUTER JOIN package_app_changeable_info ON package_app_info.app_id=package_app_changeable_info.app_id where package_app_info.app_exec='/usr/bin/launch_app' and package_app_changeable_info.app_disable IN ('false','False')
02-03 16:28:43.715+0900 I/AUL_AMD (  531): menu_db_util.h: _get_app_info_from_db_by_apppath(239) > path : /usr/bin/launch_app, ret : 0
02-03 16:28:43.720+0900 D/AUL_AMD (  531): amd_status.c: _status_get_appid_bypid(520) > second chance
02-03 16:28:43.720+0900 D/AUL_AMD (  531): amd_status.c: _status_get_appid_bypid(525) > second chance pgid = 20197, pid = 20199
02-03 16:28:43.730+0900 D/PKGMGR_INFO(  531): pkgmgrinfo_appinfo.c: pkgmgrinfo_appinfo_filter_foreach_appinfo(3737) > [SECURE_LOG] where = package_app_info.app_exec='/bin/bash' and package_app_changeable_info.app_disable IN ('false','False')
02-03 16:28:43.730+0900 D/PKGMGR_INFO(  531): pkgmgrinfo_appinfo.c: pkgmgrinfo_appinfo_filter_foreach_appinfo(3743) > [SECURE_LOG] query = select DISTINCT package_app_info.*, package_app_changeable_info.*, package_app_localized_info.app_locale, package_app_localized_info.app_label, package_app_localized_info.app_icon from package_app_info LEFT OUTER JOIN package_app_localized_info ON package_app_info.app_id=package_app_localized_info.app_id and package_app_localized_info.app_locale IN ('No Locale', 'ko-kr') LEFT OUTER JOIN package_app_app_svc ON package_app_info.app_id=package_app_app_svc.app_id LEFT OUTER JOIN package_app_app_category ON package_app_info.app_id=package_app_app_category.app_id LEFT OUTER JOIN package_app_changeable_info ON package_app_info.app_id=package_app_changeable_info.app_id where package_app_info.app_exec='/bin/bash' and package_app_changeable_info.app_disable IN ('false','False')
02-03 16:28:43.735+0900 I/AUL_AMD (  531): menu_db_util.h: _get_app_info_from_db_by_apppath(239) > path : /bin/bash, ret : 0
02-03 16:28:43.735+0900 E/AUL_AMD (  531): amd_launch.c: _start_app(1649) > no caller appid info, ret: -1
02-03 16:28:43.735+0900 W/AUL_AMD (  531): amd_launch.c: _start_app(1659) > caller pid : 20199
02-03 16:28:43.750+0900 W/AUL_AMD (  531): amd_launch.c: __nofork_processing(1138) > __nofork_processing, cmd: 0, pid: 26701
02-03 16:28:43.750+0900 D/AUL_AMD (  531): amd_launch.c: __nofork_processing(1154) > fake launch pid : 26701
02-03 16:28:43.750+0900 D/AUL     (  531): app_sock.c: __app_send_raw_with_delay_reply(421) > pid(26701) : cmd(0)
02-03 16:28:43.755+0900 D/AUL_AMD (  531): amd_launch.c: __set_reply_handler(1003) > listen fd : 29, send fd : 28
02-03 16:28:43.755+0900 D/AUL_AMD (  531): amd_launch.c: __nofork_processing(1157) > fake launch done
02-03 16:28:43.755+0900 W/AUL_AMD (  531): amd_launch.c: __reply_handler(909) > listen fd(29) , send fd(28), pid(26701), cmd(0)
02-03 16:28:43.755+0900 D/AUL     (20199): launch.c: app_request_to_launchpad(282) > launch request result : 26701
02-03 16:28:43.755+0900 D/RESOURCED(  533): proc-main.c: resourced_proc_status_change(943) > [SECURE_LOG] [resourced_proc_status_change,943] resume request 26701
02-03 16:28:43.755+0900 D/RESOURCED(  533): cpu.c: cpu_foreground_state(248) > [cpu_foreground_state,248] cpu_foreground_state : pid = 26701, appname = org.tizen.widget_viewer_sdk
02-03 16:28:43.755+0900 D/RESOURCED(  533): cgroup.c: cgroup_write_node(133) > [SECURE_LOG] [cgroup_write_node,133] cgroup_buf /sys/fs/cgroup/cpu/cgroup.procs, value 26701
02-03 16:28:43.760+0900 D/APP_CORE(26701): appcore.c: __aul_handler(448) > [APP 26701]     AUL event: AUL_START
02-03 16:28:43.760+0900 I/APP_CORE(26701): appcore-efl.c: __do_app(429) > [APP 26701] Event: RESET State: PAUSED
02-03 16:28:43.760+0900 D/APP_CORE(26701): appcore-efl.c: __do_app(461) > [APP 26701] RESET
02-03 16:28:43.760+0900 D/LAUNCH  (26701): appcore-efl.c: __do_app(463) > [widget_viewer_sdk:Application:reset:start]
02-03 16:28:43.760+0900 I/CAPI_APPFW_APPLICATION(26701): app_main.c: _ui_app_appcore_reset(645) > app_appcore_reset
02-03 16:28:43.760+0900 D/APP_SVC (26701): appsvc.c: __set_bundle(159) > __set_bundle
02-03 16:28:43.760+0900 D/WIDGET_SDK(26701): main.c: _app_control(672) > [SECURE_LOG] Loading a widget: [com.sec.starfish.clock]
02-03 16:28:43.760+0900 D/RESOURCED(  533): proc-main.c: resourced_proc_status_change(963) > [resourced_proc_status_change,963] available memory = 208
02-03 16:28:43.760+0900 D/RESOURCED(  533): freezer-process.c: freezer_process_pid_set(156) > [freezer_process_pid_set,156] freezer_process_pid_set 26701
02-03 16:28:43.760+0900 D/WIDGET_SDK(26701): main.c: unload_widget(302) > [SECURE_LOG] Unload previous widget: com.sec.starfish.clock
02-03 16:28:43.760+0900 D/WIDGET_VIEWER(26701): widget.c: _widget_set_visibility(1137) > [SECURE_LOG] [com.sec.starfish.clock] visibility is changed 0x[2]
02-03 16:28:43.760+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_del(3905) > [SECURE_LOG] Send delete request (0x1)
02-03 16:28:43.760+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: widget_unref(961) > [SECURE_LOG] refcnt: 1 (com.sec.starfish.clock)
02-03 16:28:43.765+0900 D/WIDGET_SDK(26701): main.c: prepare_widget(595) > [SECURE_LOG] Encoded content_info: [424:MGIxYmFiNjdkZGJhYWJjMTQyM2FlYmZkMzE2ZmU3OWYxAAAAAQQAAAoAAAB3aWRnZXRfaWQAFwAAAGNvbS5zZWMuc3RhcmZpc2guY2xvY2sANAAAAAEEAAASAAAAX19BVUxfU1RBUlRUSU1FX18AEgAAADE0NTQ0ODQ1MjMvNjk4MDk5AD0AAAABBAAAEQAAAF9fQVVMX1BLR19OQU1FX18AHAAAAG9yZy50aXplbi53aWRnZXRfdmlld2VyX3NkawApAAAAAQQAABMAAABfX0FVTF9DQUxMRVJfUElEX18ABgAAADIwMTk5AFIAAAABBAAAFAAAAF9fQVBQX1NWQ19PUF9UWVBFX18ALgAAAGh0dHA6Ly90aXplbi5vcmcvYXBwY29udHJvbC9vcGVyYXRpb24vZGVmYXVsdAA=]
02-03 16:28:43.765+0900 D/WIDGET_SDK(26701): main.c: prepare_widget(612) > [SECURE_LOG] [com.sec.starfish.clock] 1
02-03 16:28:43.765+0900 D/WIDGET_SDK(26701): main.c: _app_control(730) > [SECURE_LOG] Immediately loads the watch[com.sec.starfish.clock]
02-03 16:28:43.765+0900 D/WIDGET_SDK(26701): main.c: lazy_widget_loader_cb(654) > [SECURE_LOG] Lazy loader expired. load widget [com.sec.starfish.clock]
02-03 16:28:43.765+0900 D/WIDGET_SDK(26701): main.c: load_widget(497) > [SECURE_LOG] Found valid size[4]: 360x360
02-03 16:28:43.765+0900 D/WIDGET_SDK(26701): main.c: load_widget(505) > [SECURE_LOG] Resize the widget(com.sec.starfish.clock) to [4] 360x360
02-03 16:28:43.765+0900 E/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_resize(5435) > [SECURE_LOG] Failed to get box size
02-03 16:28:43.765+0900 E/WIDGET_VIEWER(26701): widget.c: widget_viewer_is_created_by_user(3452) > [SECURE_LOG] Handler is invalid
02-03 16:28:43.765+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_resize(5470) > [SECURE_LOG] Request size change: 360x360 [0x4]
02-03 16:28:43.765+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_resize(5485) > [SECURE_LOG] Create new handle: 360x360, (com.sec.starfish.clock, 424:MGIxYmFiNjdkZGJhYWJjMTQyM2FlYmZkMzE2ZmU3OWYxAAAAAQQAAAoAAAB3aWRnZXRfaWQAFwAAAGNvbS5zZWMuc3RhcmZpc2guY2xvY2sANAAAAAEEAAASAAAAX19BVUxfU1RBUlRUSU1FX18AEgAAADE0NTQ0ODQ1MjMvNjk4MDk5AD0AAAABBAAAEQAAAF9fQVVMX1BLR19OQU1FX18AHAAAAG9yZy50aXplbi53aWRnZXRfdmlld2VyX3NkawApAAAAAQQAABMAAABfX0FVTF9DQUxMRVJfUElEX18ABgAAADIwMTk5AFIAAAABBAAAFAAAAF9fQVBQX1NWQ19PUF9UWVBFX18ALgAAAGh0dHA6Ly90aXplbi5vcmcvYXBwY29udHJvbC9vcGVyYXRpb24vZGVmYXVsdAA=), user,created/default
02-03 16:28:43.770+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_resize(5506) > [SECURE_LOG] Added handle: 0x4477ae58 (0x44737ee8)
02-03 16:28:43.770+0900 D/WIDGET_SERVICE(26701): widget_service.c: update_lang_info(495) > [SECURE_LOG] Syslang is not changed: ko_KR.UTF-8
02-03 16:28:43.770+0900 D/WIDGET_SERVICE(26701): widget_service.c: widget_service_get_preview_image_path(1818) > [SECURE_LOG] Access failed: /opt/usr/apps/com.sec.starfish.clock/shared/res/kor-KR/preview.png, 2
02-03 16:28:43.770+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_overlay_loading(5324) > [SECURE_LOG] Set overlay loading (0x44737ee8) com.sec.starfish.clock
02-03 16:28:43.775+0900 D/LAUNCH  (26701): appcore-efl.c: __do_app(466) > [widget_viewer_sdk:Application:reset:done]
02-03 16:28:43.775+0900 I/APP_CORE(26701): appcore-efl.c: __do_app(479) > Legacy lifecycle: 0
02-03 16:28:43.775+0900 I/APP_CORE(26701): appcore-efl.c: __do_app(481) > [APP 26701] App already running, raise the window
02-03 16:28:43.785+0900 E/E17     (  366): e_manager.c: _e_manager_cb_client_message(1506) > ACTIVE REQUEST(0x01400002)
02-03 16:28:43.790+0900 I/APP_CORE(26701): appcore-efl.c: __do_app(485) > [APP 26701] Call the resume_cb
02-03 16:28:43.790+0900 I/CAPI_APPFW_APPLICATION(26701): app_main.c: _ui_app_appcore_resume(628) > app_appcore_resume
02-03 16:28:43.790+0900 D/APP_CORE(26701): appcore.c: __aul_handler(451) > [SECURE_LOG] caller_appid : (null)
02-03 16:28:43.790+0900 D/AUL_AMD (  531): amd_launch.c: __e17_status_handler(2189) > pid(26701) status(3)
02-03 16:28:43.790+0900 D/AUL_AMD (  531): amd_key.c: _key_ungrab(250) > _key_ungrab, win : 1800002
02-03 16:28:43.790+0900 W/AUL_AMD (  531): amd_key.c: _key_ungrab(254) > fail(-1) to ungrab key(XF86Stop)
02-03 16:28:43.790+0900 W/AUL_AMD (  531): amd_launch.c: __e17_status_handler(2194) > back key ungrab error
02-03 16:28:43.790+0900 D/AUL_AMD (  531): amd_status.c: _status_update_app_info_list(178) > pid(26701), status(0)
02-03 16:28:43.795+0900 D/RESOURCED(  533): proc-main.c: resourced_proc_status_change(875) > [SECURE_LOG] [resourced_proc_status_change,875] set foreground : 26701
02-03 16:28:43.800+0900 D/RESOURCED(  533): vmpressure-lowmem-handler.c: lowmem_move_memcgroup(1687) > [lowmem_move_memcgroup,1687] pid: 26701, proc_name: org.tizen.widget_viewer_sdk, cg_name: foreground, oom_score_adj: 200
02-03 16:28:43.800+0900 D/RESOURCED(  533): cgroup.c: cgroup_write_node(133) > [SECURE_LOG] [cgroup_write_node,133] cgroup_buf /sys/fs/cgroup/memory/foreground//cgroup.procs, value 26701
02-03 16:28:43.810+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:43.815+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:43.817
02-03 16:28:43.815+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.196600
02-03 16:28:43.815+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:43.865+0900 D/RESOURCED(  533): heart-cpu.c: heart_cpu_foreground_state(201) > [heart_cpu_foreground_state,201] heart_cpu_foreground_state : pid = 26701, appname = org.tizen.widget_viewer_sdk, pkgname = org.tizen.widget_viewer_sdk
02-03 16:28:43.865+0900 D/RESOURCED(  533): cpu.c: cpu_foreground_state(248) > [cpu_foreground_state,248] cpu_foreground_state : pid = 26701, appname = org.tizen.widget_viewer_sdk
02-03 16:28:43.865+0900 D/RESOURCED(  533): cgroup.c: cgroup_write_node(133) > [SECURE_LOG] [cgroup_write_node,133] cgroup_buf /sys/fs/cgroup/cpu/cgroup.procs, value 26701
02-03 16:28:43.880+0900 D/RESOURCED(  533): freezer-process.c: freezer_process_pid_set(156) > [freezer_process_pid_set,156] freezer_process_pid_set 26701
02-03 16:28:43.915+0900 E/WIDGET_SERVICE(26701): widget_service.c: widget_service_create_resource_lock(3556) > [SECURE_LOG] Failed to open a file /opt/usr/share/live_magazine/.widget.lck/.27263972: 13
02-03 16:28:43.915+0900 E/WIDGET_EVAS(26701): widget_viewer_evas.c: evas_render_pre_cb(6758) > [SECURE_LOG] Failed to create a resource lock
02-03 16:28:43.915+0900 E/WIDGET_SERVICE(26701): widget_service.c: widget_service_create_resource_lock(3556) > [SECURE_LOG] Failed to open a file /opt/usr/share/live_magazine/.widget.lck/.27263976: 13
02-03 16:28:43.915+0900 E/WIDGET_EVAS(26701): widget_viewer_evas.c: evas_render_pre_cb(6758) > [SECURE_LOG] Failed to create a resource lock
02-03 16:28:43.915+0900 E/WIDGET_SERVICE(26701): widget_service.c: widget_service_create_resource_lock(3556) > [SECURE_LOG] Failed to open a file /opt/usr/share/live_magazine/.widget.lck/.27263980: 13
02-03 16:28:43.915+0900 E/WIDGET_EVAS(26701): widget_viewer_evas.c: evas_render_pre_cb(6758) > [SECURE_LOG] Failed to create a resource lock
02-03 16:28:43.920+0900 D/DATA_PROVIDER_MASTER(  704): monitor.c: monitor_multicast_state_change_event(198) > [SECURE_LOG] 0 events are multicasted
02-03 16:28:43.920+0900 I/StarFish(19774): onPause
02-03 16:28:43.925+0900 D/WIDGET_VIEWER(26701): widget.c: _job_del_cb(1213) > [SECURE_LOG] Send delete request
02-03 16:28:43.925+0900 I/CAPI_WIDGET_APPLICATION(19774): widget_app.c: __provider_pause_cb(296) > widget obj was paused
02-03 16:28:43.925+0900 W/W_HOME  (  729): event_manager.c: _ecore_x_message_cb(403) > state: 0 -> 1
02-03 16:28:43.925+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:4, app_state:1 win_state:1(1) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.925+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:2, app_state:1 win_state:1(1) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.930+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:1, app_state:1 win_state:1(1) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.930+0900 W/W_HOME  (  729): main.c: _ecore_x_message_cb(1222) > main_info.is_win_on_top: 0
02-03 16:28:43.930+0900 I/GESTURE (  139): gesture.c: BackGestureSetProperty(4502) > [BackGestureSetProperty] atom=_E_MOVE_W_HOME_CLOCK_STATE, value=0, No clock display 
02-03 16:28:43.950+0900 I/MALI    (  729): egl_platform_x11_tizen.c: tizen_update_native_surface_private(194) > [EGL-X11] surface->[0x44357558] swap changed from sync to async
02-03 16:28:43.965+0900 D/DATA_PROVIDER_MASTER(  704): server.c: client_activate_package(6306) > [SECURE_LOG] pid[26701] pkgname[com.sec.starfish.clock]
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _window_visibility_cb(448) > Window [0x2600003] is now visible(1)
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _window_visibility_cb(458) > state: 1 -> 0
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:4, app_state:1 win_state:1(0) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.995+0900 W/W_HOME  (  729): main.c: _window_visibility_cb(1189) > Window [0x2600003] is now visible(1)
02-03 16:28:43.995+0900 D/APP_CORE(  729): appcore-efl.c: __update_win(673) > [EVENT_TEST][EVENT] __update_win WIN:2600003 fully_obscured 1
02-03 16:28:43.995+0900 D/APP_CORE(  729): appcore-efl.c: __visibility_cb(846) > bvisibility 0, b_active 1
02-03 16:28:43.995+0900 D/APP_CORE(  729): appcore-efl.c: __visibility_cb(861) >  Go to Pasue state 
02-03 16:28:43.995+0900 I/APP_CORE(  729): appcore-efl.c: __do_app(429) > [APP 729] Event: PAUSE State: RUNNING
02-03 16:28:43.995+0900 D/APP_CORE(  729): appcore-efl.c: __do_app(503) > [APP 729] PAUSE
02-03 16:28:43.995+0900 I/CAPI_APPFW_APPLICATION(  729): app_main.c: app_appcore_pause(202) > app_appcore_pause
02-03 16:28:43.995+0900 W/W_HOME  (  729): main.c: _appcore_pause_cb(690) > appcore pause
02-03 16:28:43.995+0900 D/W_HOME  (  729): main.c: _pause_cb(665) > Paused
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _app_pause_cb(372) > state: 1 -> 2
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:2, app_state:2 win_state:1(0) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:0, app_state:2 win_state:1(0) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.995+0900 D/W_HOME  (  729): main.c: home_pause(748) > Home Paused
02-03 16:28:43.995+0900 W/W_HOME  (  729): main.c: home_pause(751) > clock/widget paused
02-03 16:28:43.995+0900 D/W_HOME  (  729): clock_view.c: clock_view_event_handler(573) > event:10002 received
02-03 16:28:43.995+0900 W/W_HOME  (  729): event_manager.c: _state_control(194) > control:1, app_state:2 win_state:1(0) pm_state:1 home_visible:1 clock_visible:1 tutorial_state:0 editing : 0, home_clocklist:0, addviewer:0 scrolling : 0, powersaving : 0, apptray state : 1, apptray visibility : 0, apptray edit visibility : 0
02-03 16:28:43.995+0900 D/W_HOME  (  729): page.c: page_access_highlight(824) > focus set to 0x4a0a23a8
02-03 16:28:43.995+0900 I/MESSAGE_PORT(  729): message_port.cpp: message_port_check_remote_port(189) > [SECURE_LOG] Check remote port (com.samsung.w-music-player.music-control-service):(music-control-service-request-message-port).
02-03 16:28:43.995+0900 I/MESSAGE_PORT(  729): MessagePortProxy.cpp: CheckRemotePort(331) > [SECURE_LOG] Check a remote port : [com.samsung.w-music-player.music-control-service:music-control-service-request-message-port]
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: OnReadMessage(739) > _MessagePortIpcServer::OnReadMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: HandleReceivedMessage(578) > _MessagePortIpcServer::HandleReceivedMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: OnIpcRequestReceived(147) > MessagePort message received
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: OnCheckRemotePort(115) > _MessagePortStub::OnCheckRemotePort.
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: CheckRemotePort(207) > _MessagePortService::CheckRemotePort
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: GetKey(365) > _MessagePortService::GetKey
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: GetKey(395) > [SECURE_LOG] _MessagePortService::GetKey Key:[com.samsung.w-music-player.music-control-service:music-control-service-request-message-port]
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: CheckRemotePort(220) > Check a remote message port: [com.samsung.w-music-player.music-control-service:music-control-service-request-message-port]
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: Send(847) > _MessagePortIpcServer::Stop
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  729): message_port.cpp: message_port_send_message(211) > [SECURE_LOG] Send a message to the remote port (com.samsung.w-music-player.music-control-service):(music-control-service-request-message-port).
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  729): MessagePortProxy.cpp: SendMessage(403) > [SECURE_LOG] Send a message to : [com.samsung.w-music-player.music-control-service:music-control-service-request-message-port]
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: OnReadMessage(739) > _MessagePortIpcServer::OnReadMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: HandleReceivedMessage(578) > _MessagePortIpcServer::HandleReceivedMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: OnIpcRequestReceived(147) > MessagePort message received
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: OnSendMessage(126) > MessagePort OnSendMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: SendMessage(291) > _MessagePortService::SendMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: GetKey(365) > _MessagePortService::GetKey
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: GetKey(395) > [SECURE_LOG] _MessagePortService::GetKey Key:[com.samsung.w-music-player.music-control-service:music-control-service-request-message-port]
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortService.cpp: SendMessage(299) > Sends a message to a remote message port [com.samsung.w-music-player.music-control-service:music-control-service-request-message-port]
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortStub.cpp: SendMessage(138) > MessagePort SendMessage
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: SendResponse(884) > _MessagePortIpcServer::SendResponse
02-03 16:28:44.000+0900 D/AUL_AMD (  531): amd_status.c: _status_update_app_info_list(178) > pid(729), status(1)
02-03 16:28:44.000+0900 I/MESSAGE_PORT( 1118): MessagePortProxy.cpp: OnSendMessageInternal(723) > [SECURE_LOG] Message received! from App: com.samsung.w-home, to App: com.samsung.w-music-player.music-control-service, Port: music-control-service-request-message-port, Trusted: FALSE
02-03 16:28:44.000+0900 I/MESSAGE_PORT( 1118): message_port.cpp: message_dispatcher(52) > [SECURE_LOG] A message has been received to specific local port id (1) from remote port (com.samsung.w-home):((null)).
02-03 16:28:44.000+0900 D/APP_SVC ( 1118): appsvc.c: __set_bundle(159) > __set_bundle
02-03 16:28:44.000+0900 D/MUSIC_CONTROL_SERVICE( 1118): music-control-service.c: _music_control_service_bundle_foreach_cb(555) > [32m[TID:1118]   key:msgId, val:music-messageport-req[0m
02-03 16:28:44.000+0900 D/APP_SVC ( 1118): appsvc.c: __set_bundle(159) > __set_bundle
02-03 16:28:44.000+0900 D/MUSIC_CONTROL_SERVICE( 1118): music-control-service.c: _music_control_service_bundle_foreach_cb(555) > [32m[TID:1118]   key:value, val:false[0m
02-03 16:28:44.000+0900 E/CAPI_APPFW_APP_CONTROL( 1118): app_control.c: app_control_error(133) > [app_control_get_caller] INVALID_PARAMETER(0xffffffea) : invalid app_control handle type
02-03 16:28:44.000+0900 D/MUSIC_CONTROL_SERVICE( 1118): music-control-service.c: _music_control_service_pasre_request(259) > [32m[TID:1118]   msg port[0m
02-03 16:28:44.000+0900 W/MUSIC_CONTROL_SERVICE( 1118): music-control-service.c: _music_control_service_pasre_request(263) > [SECURE_LOG] [T:1118] caller = com.samsung.w-home
02-03 16:28:44.000+0900 W/MUSIC_CONTROL_SERVICE( 1118): music-control-service.c: _music_control_service_pasre_request(409) > [33m[TID:1118]   value = [false][0m
02-03 16:28:44.000+0900 I/MESSAGE_PORT(  444): MessagePortIpcServer.cpp: Send(847) > _MessagePortIpcServer::Stop
02-03 16:28:44.005+0900 D/W_HOME  (  729): clock_shortcut.c: _mp_messageport_register(153) > requested a music state via message port
02-03 16:28:44.005+0900 D/APP_CORE(  729): appcore-efl.c: __trm_app_info_send_socket(233) > __trm_app_info_send_socket
02-03 16:28:44.005+0900 D/APP_CORE(  729): appcore-efl.c: __trm_app_info_send_socket(260) > send
02-03 16:28:44.005+0900 D/DATA_PROVIDER_MASTER(  704): server.c: client_new(1411) > [SECURE_LOG] pid[26701] period[57178.515625] pkgname[com.sec.starfish.clock] content[424:MGIxYmFiNjdkZGJhYWJjMTQyM2FlYmZkMzE2ZmU3OWYxAAAAAQQAAAoAAAB3aWRnZXRfaWQAFwAAAGNvbS5zZWMuc3RhcmZpc2guY2xvY2sANAAAAAEEAAASAAAAX19BVUxfU1RBUlRUSU1FX18AEgAAADE0NTQ0ODQ1MjMvNjk4MDk5AD0AAAABBAAAEQAAAF9fQVVMX1BLR19OQU1FX18AHAAAAG9yZy50aXplbi53aWRnZXRfdmlld2VyX3NkawApAAAAAQQAABMAAABfX0FVTF9DQUxMRVJfUElEX18ABgAAADIwMTk5AFIAAAABBAAAFAAAAF9fQVBQX1NWQ19PUF9UWVBFX18ALgAAAGh0dHA6Ly90aXplbi5vcmcvYXBwY29udHJvbC9vcGVyYXRpb24vZGVmYXVsdAA=] cluster[user,created] category[default] period[-1.000000]
02-03 16:28:44.010+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:44.010+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:44.16
02-03 16:28:44.010+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.196800
02-03 16:28:44.010+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:44.015+0900 D/APP_CORE(26701): appcore-efl.c: __update_win(673) > [EVENT_TEST][EVENT] __update_win WIN:1400002 fully_obscured 0
02-03 16:28:44.015+0900 D/APP_CORE(26701): appcore-efl.c: __visibility_cb(846) > bvisibility 1, b_active 0
02-03 16:28:44.015+0900 D/APP_CORE(26701): appcore-efl.c: __visibility_cb(849) >  Go to Resume state
02-03 16:28:44.015+0900 I/APP_CORE(26701): appcore-efl.c: __do_app(429) > [APP 26701] Event: RESUME State: RUNNING
02-03 16:28:44.015+0900 D/LAUNCH  (26701): appcore-efl.c: __do_app(519) > [widget_viewer_sdk:Application:resume:start]
02-03 16:28:44.015+0900 D/LAUNCH  (26701): appcore-efl.c: __do_app(529) > [widget_viewer_sdk:Application:resume:done]
02-03 16:28:44.015+0900 D/APP_CORE(26701): appcore-efl.c: __trm_app_info_send_socket(233) > __trm_app_info_send_socket
02-03 16:28:44.020+0900 D/APP_CORE(  729): appcore-efl.c: _capture_and_make_file(1571) > [SECURE_LOG] Capture : win[2600003] -> redirected win[60005b] for com.samsung.w-home[729]
02-03 16:28:44.020+0900 D/APP_CORE(26701): appcore-efl.c: __trm_app_info_send_socket(260) > send
02-03 16:28:44.050+0900 I/MUSIC_CONTROL_SERVICE( 1118): music-control-client-manager.c: music_control_client_mgr_remove_client_app(319) > [SECURE_LOG] [T:1118] [com.samsung.w-home] removed
02-03 16:28:44.050+0900 D/MUSIC_CONTROL_SERVICE( 1118): music-control-client-manager.c: music_control_client_mgr_remove_client_app(321) > [32m[TID:1118]   client count = 0[0m
02-03 16:28:44.055+0900 D/DATA_PROVIDER_MASTER(  704): util.c: util_free_space(150) > [SECURE_LOG] Available size: 2023354368, f_bsize: 4096, f_bavail: 0
02-03 16:28:44.055+0900 D/DATA_PROVIDER_MASTER(  704): instance.c: instance_create(971) > [SECURE_LOG] Extra Bundle Data extracted: [MGIxYmFiNjdkZGJhYWJjMTQyM2FlYmZkMzE2ZmU3OWYxAAAAAQQAAAoAAAB3aWRnZXRfaWQAFwAAAGNvbS5zZWMuc3RhcmZpc2guY2xvY2sANAAAAAEEAAASAAAAX19BVUxfU1RBUlRUSU1FX18AEgAAADE0NTQ0ODQ1MjMvNjk4MDk5AD0AAAABBAAAEQAAAF9fQVVMX1BLR19OQU1FX18AHAAAAG9yZy50aXplbi53aWRnZXRfdmlld2VyX3NkawApAAAAAQQAABMAAABfX0FVTF9DQUxMRVJfUElEX18ABgAAADIwMTk5AFIAAAABBAAAFAAAAF9fQVBQX1NWQ19PUF9UWVBFX18ALgAAAGh0dHA6Ly90aXplbi5vcmcvYXBwY29udHJvbC9vcGVyYXRpb24vZGVmYXVsdAA=]
02-03 16:28:44.055+0900 D/DATA_PROVIDER_MASTER(  704): slave_life.c: slave_load_instance(1712) > [SECURE_LOG] Instance: (19774)2
02-03 16:28:44.055+0900 D/DATA_PROVIDER_MASTER(  704): xmonitor.c: xmonitor_resume(339) > [SECURE_LOG] 26701 is resumed
02-03 16:28:44.055+0900 D/DATA_PROVIDER_MASTER(  704): client_life.c: client_is_all_paused(479) > [SECURE_LOG] 2, 0
02-03 16:28:44.060+0900 D/WIDGET_PROVIDER_APP(19774): client.c: method_new(479) > [SECURE_LOG] Create: pkgname[com.sec.starfish.clock], id[file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png], content[], timeout[60], has_script[0], period[0.000000], cluster[user,created], category[default], skip[1], abi[app], size: 360x360, ori: 0
02-03 16:28:44.065+0900 E/COM_CORE(19774): secure_socket.c: secure_socket_create_client(289) > [SECURE_LOG] Failed to connect to server [/opt/usr/share/live_magazine//.26701.18711.496094.widget.viewer] Permission denied
02-03 16:28:44.065+0900 D/WIDGET_PROVIDER_APP(19774): connection.c: connection_create(161) > [SECURE_LOG] Try to get a connection through SHARED_SOCKET
02-03 16:28:44.070+0900 E/COM_CORE(19774): secure_socket.c: secure_socket_create_client(289) > [SECURE_LOG] Failed to connect to server [/tmp/.data-provider-master-fd.socket] Permission denied
02-03 16:28:44.070+0900 E/WIDGET_PROVIDER_APP(19774): connection.c: connection_create(183) > [SECURE_LOG] Cannot retrieve the result packet
02-03 16:28:44.070+0900 E/WIDGET_PROVIDER_APP(19774): connection.c: connection_create(191) > [SECURE_LOG] Unable to make a connection /opt/usr/share/live_magazine//.26701.18711.496094.widget.viewer
02-03 16:28:44.070+0900 E/WIDGET_PROVIDER_APP(19774): client.c: instance_create(371) > [SECURE_LOG] Failed to create a new connection
02-03 16:28:44.080+0900 D/WIDGET_PROVIDER(19774): widget_provider_buffer.c: widget_provider_buffer_create(2012) > [SECURE_LOG] acquire_buffer: [WIDGET] file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png, handler: 0x40881f35
02-03 16:28:44.080+0900 D/WIDGET  (19774): binder.c: binder_ecore_evas_new(537) > [SECURE_LOG] Preferred engine: (null) (opengl or opengl_x11)
02-03 16:28:44.080+0900 D/WIDGET  (19774): binder.c: binder_ecore_evas_new(549) > [SECURE_LOG] Preferred engine: (null) (opengl)
02-03 16:28:44.080+0900 D/WIDGET_PROVIDER(19774): widget_provider_buffer.c: widget_provider_buffer_acquire(2084) > [SECURE_LOG] acquire_buffer: [WIDGET] file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png, 1x1, size: 4, handler: 0x40881f35
02-03 16:28:44.080+0900 D/COM_CORE(  704): com-core_thread.c: tcb_create(564) > [SECURE_LOG] [85] New TCB created: R(86), W(88)
02-03 16:28:44.080+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(350) > [SECURE_LOG] Thread is created for 85 (server: 4)
02-03 16:28:44.080+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: buffer_handler_resize(1060) > [SECURE_LOG] Buffer size is updated[1x1]
02-03 16:28:44.080+0900 D/WIDGET_SERVICE(  704): widget_service.c: widget_service_create_resource_lock(3534) > [SECURE_LOG] Create a file named [/opt/usr/share/live_magazine/.widget.lck/.27263982]
02-03 16:28:44.080+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: load_pixmap_buffer(552) > [SECURE_LOG] Loaded pixmap(info->id): pixmap://27263982:4
02-03 16:28:44.090+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(655) > [SECURE_LOG] Recv'd size: 64 (header: 64) pid: 704, fd: -1
02-03 16:28:44.090+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(701) > [SECURE_LOG] Recv'd 24 bytes (pid: 704), fd: -1
02-03 16:28:44.090+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(718) > [SECURE_LOG] Close connection: 35 (recv_fd: -1)
02-03 16:28:44.090+0900 D/WIDGET_PROVIDER(19774): fb.c: fb_create(650) > [SECURE_LOG] PIXMAP: 27263982
02-03 16:28:44.090+0900 D/WIDGET_PROVIDER(19774): widget_provider_buffer.c: send_acquire_request(148) > [SECURE_LOG] type: 0x0, name: 23_54471.541847, pkgname[com.sec.starfish.clock], id[file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png], w[1], h[1], size[4], buffer_id[pixmap://27263982:4], fb[0x42e18b30]
02-03 16:28:44.090+0900 E/COM_CORE(  704): com-core_thread.c: client_cb(397) > [SECURE_LOG] Available data: 0
02-03 16:28:44.090+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(433) > [SECURE_LOG] Client CB is terminated (85)
02-03 16:28:44.090+0900 D/COM_CORE(  704): com-core_thread.c: com_core_thread_recv_with_fd(1103) > [SECURE_LOG] Disconnected
02-03 16:28:44.090+0900 D/COM_CORE(  704): com-core_packet.c: service_cb(460) > [SECURE_LOG] ZERO bytes receives(19774)
02-03 16:28:44.090+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(498) > [SECURE_LOG] Service callback returns -104 < 0
02-03 16:28:44.090+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(505) > [SECURE_LOG] Disconnecting
02-03 16:28:44.090+0900 D/COM_CORE(  704): com-core_packet.c: client_disconnected_cb(361) > [SECURE_LOG] Clean up all requests and a receive context for handle(85) for pid(-1)
02-03 16:28:44.090+0900 E/COM_CORE(  704): com-core_thread.c: terminate_thread(192) > [SECURE_LOG] Thread returns: -104
02-03 16:28:44.090+0900 D/WIDGET_PROVIDER(19774): fb.c: create_gem(495) > [SECURE_LOG] dri2_buffer: 0x42e19c60, name: 0x24, 1x1 (1x1), pitch: 8, buf_count: 1, gem: 0x42e19048
02-03 16:28:44.090+0900 D/WIDGET  (19774): binder.c: alloc_fb(445) > [SECURE_LOG] HW Accelerated buffer is created 0x42e18ef0, (1x1)
02-03 16:28:44.090+0900 D/WIDGET_PROVIDER(19774): fb.c: fb_stride(1053) > [SECURE_LOG] Stride: 8
02-03 16:28:44.090+0900 D/WIDGET  (19774): binder.c: alloc_stride_fb(480) > [SECURE_LOG] bpp: 32, stride: 8
02-03 16:28:44.095+0900 I/wnotib  (  729): w-notification-board-broker-main.c: _wnotib_ecore_x_event_visibility_changed_cb(701) > fully_obscured: 1
02-03 16:28:44.095+0900 D/WIDGET_PROVIDER(19774): fb.c: destroy_gem(510) > [SECURE_LOG] unref pixmap bo
02-03 16:28:44.095+0900 E/wnotib  (  729): w-notification-board-action-drawer.c: wnotib_action_drawer_hidden_get(4570) > [NULL==g_wnotib_action_drawer_data] msg Drawer not initialized.
02-03 16:28:44.095+0900 D/WIDGET  (19774): binder.c: free_fb(496) > [SECURE_LOG] HW Accelerated buffer is destroyed
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: tcb_create(564) > [SECURE_LOG] [85] New TCB created: R(86), W(88)
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(350) > [SECURE_LOG] Thread is created for 85 (server: 4)
02-03 16:28:44.095+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: destroy_pixmap(351) > [SECURE_LOG] pixmap 27263982
02-03 16:28:44.095+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(655) > [SECURE_LOG] Recv'd size: 64 (header: 64) pid: 704, fd: -1
02-03 16:28:44.095+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(701) > [SECURE_LOG] Recv'd 4 bytes (pid: 704), fd: -1
02-03 16:28:44.095+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(718) > [SECURE_LOG] Close connection: 35 (recv_fd: -1)
02-03 16:28:44.095+0900 D/WIDGET  (19774): widget.c: widget_viewer_release_buffer(609) > [SECURE_LOG] Release buffer: -1 (0)
02-03 16:28:44.095+0900 D/WIDGET  (19774): binder.c: alloc_fb(424) > [SECURE_LOG] Size of ee is updated: 360x360 (info: 0x42e18ef0)
02-03 16:28:44.095+0900 D/WIDGET_PROVIDER(19774): widget_provider_buffer.c: widget_provider_buffer_acquire(2084) > [SECURE_LOG] acquire_buffer: [WIDGET] file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png, 360x360, size: 4, handler: 0x40881f35
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: tcb_create(564) > [SECURE_LOG] [97] New TCB created: R(98), W(99)
02-03 16:28:44.095+0900 E/COM_CORE(  704): com-core_thread.c: client_cb(397) > [SECURE_LOG] Available data: 0
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(433) > [SECURE_LOG] Client CB is terminated (85)
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: com_core_thread_recv_with_fd(1103) > [SECURE_LOG] Disconnected
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_packet.c: service_cb(460) > [SECURE_LOG] ZERO bytes receives(564)
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(498) > [SECURE_LOG] Service callback returns -104 < 0
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(505) > [SECURE_LOG] Disconnecting
02-03 16:28:44.095+0900 D/COM_CORE(  704): com-core_packet.c: client_disconnected_cb(361) > [SECURE_LOG] Clean up all requests and a receive context for handle(85) for pid(-1)
02-03 16:28:44.095+0900 E/COM_CORE(  704): com-core_thread.c: terminate_thread(192) > [SECURE_LOG] Thread returns: -104
02-03 16:28:44.100+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(350) > [SECURE_LOG] Thread is created for 97 (server: 4)
02-03 16:28:44.100+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: buffer_handler_resize(1060) > [SECURE_LOG] Buffer size is updated[360x360]
02-03 16:28:44.100+0900 D/WIDGET_SERVICE(  704): widget_service.c: widget_service_create_resource_lock(3534) > [SECURE_LOG] Create a file named [/opt/usr/share/live_magazine/.widget.lck/.27263984]
02-03 16:28:44.100+0900 D/DATA_PROVIDER_MASTER(  704): buffer_handler.c: load_pixmap_buffer(552) > [SECURE_LOG] Loaded pixmap(info->id): pixmap://27263984:4
02-03 16:28:44.100+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(655) > [SECURE_LOG] Recv'd size: 64 (header: 64) pid: 704, fd: -1
02-03 16:28:44.100+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(701) > [SECURE_LOG] Recv'd 24 bytes (pid: 704), fd: -1
02-03 16:28:44.100+0900 D/COM_CORE(19774): com-core_packet.c: com_core_packet_oneshot_send(718) > [SECURE_LOG] Close connection: 35 (recv_fd: -1)
02-03 16:28:44.100+0900 D/WIDGET_PROVIDER(19774): fb.c: fb_create(650) > [SECURE_LOG] PIXMAP: 27263984
02-03 16:28:44.100+0900 D/WIDGET_PROVIDER(19774): widget_provider_buffer.c: send_acquire_request(148) > [SECURE_LOG] type: 0x0, name: 23_54471.541847, pkgname[com.sec.starfish.clock], id[file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png], w[360], h[360], size[4], buffer_id[pixmap://27263984:4], fb[0x42dff540]
02-03 16:28:44.105+0900 D/WIDGET_PROVIDER(19774): fb.c: create_gem(495) > [SECURE_LOG] dri2_buffer: 0x42e15e60, name: 0x24, 360x360 (360x360), pitch: 1440, buf_count: 1, gem: 0x42e24508
02-03 16:28:44.105+0900 D/WIDGET  (19774): binder.c: alloc_fb(445) > [SECURE_LOG] HW Accelerated buffer is created 0x42e18ef0, (360x360)
02-03 16:28:44.105+0900 D/WIDGET_PROVIDER(19774): fb.c: fb_stride(1053) > [SECURE_LOG] Stride: 1440
02-03 16:28:44.105+0900 D/WIDGET  (19774): binder.c: alloc_stride_fb(480) > [SECURE_LOG] bpp: 32, stride: 1440
02-03 16:28:44.105+0900 E/COM_CORE(  704): com-core_thread.c: client_cb(397) > [SECURE_LOG] Available data: 0
02-03 16:28:44.105+0900 D/COM_CORE(  704): com-core_thread.c: client_cb(433) > [SECURE_LOG] Client CB is terminated (97)
02-03 16:28:44.105+0900 D/COM_CORE(  704): com-core_thread.c: com_core_thread_recv_with_fd(1103) > [SECURE_LOG] Disconnected
02-03 16:28:44.105+0900 D/COM_CORE(  704): com-core_packet.c: service_cb(460) > [SECURE_LOG] ZERO bytes receives(24254)
02-03 16:28:44.105+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(498) > [SECURE_LOG] Service callback returns -104 < 0
02-03 16:28:44.105+0900 D/COM_CORE(  704): com-core_thread.c: evt_pipe_cb(505) > [SECURE_LOG] Disconnecting
02-03 16:28:44.105+0900 D/COM_CORE(  704): com-core_packet.c: client_disconnected_cb(361) > [SECURE_LOG] Clean up all requests and a receive context for handle(97) for pid(-1)
02-03 16:28:44.105+0900 E/COM_CORE(  704): com-core_thread.c: terminate_thread(192) > [SECURE_LOG] Thread returns: -104
02-03 16:28:44.105+0900 D/DATA_PROVIDER_MASTER(  704): xmonitor.c: xmonitor_pause(331) > [SECURE_LOG] 729 is paused
02-03 16:28:44.105+0900 D/DATA_PROVIDER_MASTER(  704): client_life.c: client_is_all_paused(479) > [SECURE_LOG] 2, 1
02-03 16:28:44.130+0900 D/WIDGET_VIEWER(26701): client.c: master_update_id(1651) > [SECURE_LOG] Update ID(file:///opt/usr/share/live_magazine/com.sec.starfish.clock_26701_57178.515625.png) for 57178.515625
02-03 16:28:44.195+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_event_widget_size_changed(6242) > [SECURE_LOG] widget LB size is changed
02-03 16:28:44.210+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(920) > state: 2
02-03 16:28:44.210+0900 D/WATCH_CORE(24254): appcore-watch.c: __get_timeinfo(782) > Current time: 2-3 16:28:44.213
02-03 16:28:44.210+0900 D/WATCH_CORE(24254): appcore-watch.c: __watch_core_time_tick(934) > next time tick: 0.197400
02-03 16:28:44.210+0900 I/CAPI_WATCH_APPLICATION(24254): watch_app_main.c: _watch_core_time_tick(306) > _watch_core_time_tick
02-03 16:28:44.265+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_event_widget_size_changed(6242) > [SECURE_LOG] widget LB size is changed
02-03 16:28:44.285+0900 D/APP_CORE(26701): appcore.c: __prt_ltime(214) > [APP 26701] first idle after reset: 87675441 msec
02-03 16:28:44.325+0900 D/WIDGET_EVAS(26701): widget_viewer_evas.c: __widget_animation_done_cb(3472) > [SECURE_LOG] Animation finished
02-03 16:28:44.380+0900 D/AUL_AMD (  531): amd_launch.c: __send_cpu_boost(467) > __send_cpu_boost enter
02-03 16:28:44.380+0900 D/AUL_AMD (  531): amd_launch.c: __send_cpu_boost(492) > __send_cpu_boost ok
02-03 16:28:44.380+0900 D/LAUNCH  (  531): amd_launch.c: __e17_status_handler(2214) > [widget_viewer_sdk:Application:Launching:done]
02-03 16:28:44.385+0900 W/CRASH_MANAGER(20204): worker.c: worker_job(1199) > 1119774636c6f145448452
