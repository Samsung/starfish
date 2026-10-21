http://web-platform.test:8000/websockets/extended-payload-length.html
http://web-platform.test:8000/websockets/binary/001.html
http://web-platform.test:8000/websockets/binary/002.html
http://web-platform.test:8000/websockets/binary/004.html
http://web-platform.test:8000/websockets/binary/005.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/001.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/002.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/003.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/004.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/005.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/006.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/url/resolve.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/001.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/002.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/003.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/004.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/005.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/006.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/007.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/008.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/009.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/010.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/011.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/send/012.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/001.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/002.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/003.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/004.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/005.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/006.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/007.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/readyState/008.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/protocol/protocol-initial.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/extensions/001.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/001.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/002.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/004.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/006.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/010.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/011.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/012.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/013.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/014.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/015.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/016.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/017.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/019.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/events/020.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/constants/001.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/constants/002.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/constants/003.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/constants/004.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/constants/005.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/constants/006.html
# [auto-fail] http://web-platform.test:8000/websockets/interfaces/WebSocket/close/close-basic.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/close/close-multiple.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/close/close-replace.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/close/close-return.html
# Timing-dependent until bufferedAmount is tracked per event-loop task:
# the socket thread decrements the count as soon as a frame is written,
# so an assert right after send() can observe 0 under parallel load.
# http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-arraybuffer.html
# http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-blob.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-defineProperty-getter.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-defineProperty-setter.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-deleting.html
# [auto-fail] http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-getting.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-initial.html
# http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-large.html
http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-readonly.html
# http://web-platform.test:8000/websockets/interfaces/WebSocket/bufferedAmount/bufferedAmount-unicode.html
http://web-platform.test:8000/websockets/interfaces/CloseEvent/constructor.html
http://web-platform.test:8000/websockets/interfaces/CloseEvent/historical.html
http://web-platform.test:8000/websockets/multi-globals/message-received.html
# [auto-fail] http://web-platform.test:8000/websockets/security/002.html
http://web-platform.test:8000/websockets/opening-handshake/001.html
http://web-platform.test:8000/websockets/opening-handshake/002.html
# [auto-fail] http://web-platform.test:8000/websockets/keeping-connection-open/001.html
http://web-platform.test:8000/websockets/constructor/001.html
http://web-platform.test:8000/websockets/constructor/004.html
http://web-platform.test:8000/websockets/constructor/005.html
http://web-platform.test:8000/websockets/constructor/007.html
http://web-platform.test:8000/websockets/constructor/008.html
http://web-platform.test:8000/websockets/constructor/009.html
http://web-platform.test:8000/websockets/constructor/010.html
http://web-platform.test:8000/websockets/constructor/011.html
http://web-platform.test:8000/websockets/constructor/012.html
http://web-platform.test:8000/websockets/constructor/013.html
http://web-platform.test:8000/websockets/constructor/016.html
http://web-platform.test:8000/websockets/constructor/017.html
http://web-platform.test:8000/websockets/constructor/018.html
http://web-platform.test:8000/websockets/constructor/019.html
http://web-platform.test:8000/websockets/constructor/020.html
http://web-platform.test:8000/websockets/constructor/021.html
http://web-platform.test:8000/websockets/Create-asciiSep-protocol-string.any.html
http://web-platform.test:8000/websockets/Create-asciiSep-protocol-string.any.worker.html
# Bad-port blocking (Fetch "port blocking") is not implemented: the error
# event only comes from the TCP connect failing, so the result depends on
# how fast the host refuses closed ports (instant pass vs harness timeout).
# http://web-platform.test:8000/websockets/Create-blocked-port.any.html
# [auto-fail:TIMEOUT] http://web-platform.test:8000/websockets/Create-blocked-port.any.worker.html
http://web-platform.test:8000/websockets/Create-extensions-empty.any.html
http://web-platform.test:8000/websockets/Create-extensions-empty.any.worker.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-http-urls.any.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-http-urls.any.worker.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-invalid-urls.any.html
# [auto-fail:NO_COMPLETION] http://web-platform.test:8000/websockets/Create-invalid-urls.any.worker.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-non-absolute-url.any.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-non-absolute-url.any.worker.html
http://web-platform.test:8000/websockets/Create-nonAscii-protocol-string.any.html
http://web-platform.test:8000/websockets/Create-nonAscii-protocol-string.any.worker.html
http://web-platform.test:8000/websockets/Create-on-worker-shutdown.any.html
http://web-platform.test:8000/websockets/Create-on-worker-shutdown.any.worker.html
http://web-platform.test:8000/websockets/Create-protocol-with-space.any.html
http://web-platform.test:8000/websockets/Create-protocol-with-space.any.worker.html
http://web-platform.test:8000/websockets/Create-protocols-repeated-case-insensitive.any.html
http://web-platform.test:8000/websockets/Create-protocols-repeated-case-insensitive.any.worker.html
http://web-platform.test:8000/websockets/Create-protocols-repeated.any.html
http://web-platform.test:8000/websockets/Create-protocols-repeated.any.worker.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-url-with-space.any.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-url-with-space.any.worker.html
# [auto-fail:HARNESS_STATUS_1] http://web-platform.test:8000/websockets/Create-valid-url-array-protocols.any.html
# [auto-fail:HARNESS_STATUS_2] http://web-platform.test:8000/websockets/Create-valid-url-array-protocols.any.worker.html
http://web-platform.test:8000/websockets/Create-valid-url-binaryType-blob.any.html
http://web-platform.test:8000/websockets/Create-valid-url-binaryType-blob.any.worker.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-valid-url-protocol-empty.any.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/Create-valid-url-protocol-empty.any.worker.html
http://web-platform.test:8000/websockets/Create-valid-url-protocol-setCorrectly.any.html
http://web-platform.test:8000/websockets/Create-valid-url-protocol-setCorrectly.any.worker.html
http://web-platform.test:8000/websockets/Create-valid-url-protocol-string.any.html
http://web-platform.test:8000/websockets/Create-valid-url-protocol-string.any.worker.html
http://web-platform.test:8000/websockets/Create-valid-url-protocol.any.html
http://web-platform.test:8000/websockets/Create-valid-url-protocol.any.worker.html
http://web-platform.test:8000/websockets/Create-valid-url.any.html
http://web-platform.test:8000/websockets/Create-valid-url.any.worker.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/constructor/006.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/constructor/014.html
# [auto-fail:SUBTESTS_FAILED] http://web-platform.test:8000/websockets/constructor/022.html
