#!/bin/sh
# Manual WebRTC TCs
ln -fs ../../internal-test/webrtc test/cairo/reftest/web_platform_test/webrtc_internal

# auto
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCConfiguration-bundlePolicy.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCConfiguration-iceTransportPolicy.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCConfiguration-rtcpMuxPolicy.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCIceCandidate-constructor.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnectionIceEvent-constructor.html'

# manual
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-createAnswer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-createOffer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setLocalDescription-offer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription-offer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setLocalDescription-pranswer.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setLocalDescription-answer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription-answer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription-pranswer.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setLocalDescription.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-ontrack.https.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription-replaceTrack.https.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-removeTrack.https.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription-tracks.https.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCConfiguration-iceServers.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-onnegotiationneeded.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-addIceCandidate.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-createOffer-offerToReceive.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-addTrack.https.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-createDataChannel.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-connectionState.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-constructor.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-iceConnectionState.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-iceGatheringState.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-ondatachannel.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-setRemoteDescription.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-add-track-no-deadlock.https.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCDataChannel-id.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-getDefaultIceServers.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCPeerConnection-getTransceivers.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCSctpTransport-maxMessageSize.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/historical.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/interfaces.https.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/promises-call.html'

./Starfish 'test/cairo/internal-test/webrtc/webrtc01_localConnections.html'

./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCSctpTransport-constructor-internal.html'
./Starfish 'http://web-platform.test:8000/webrtc_internal/RTCRtpTransceiver-setDirection-internal.html'
