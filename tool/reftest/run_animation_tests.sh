#!/bin/sh

# web platform tests
./Starfish 'test/cairo/internal-test/css/animation/parse-animation.html'

# manual animation tc
./Starfish 'test/cairo/internal-test/animation/a1.html'
./Starfish 'test/cairo/internal-test/animation/a2.html'
./Starfish 'test/cairo/internal-test/animation/a3.html'
./Starfish 'test/cairo/internal-test/animation/a4.html'
./Starfish 'test/cairo/internal-test/animation/a5.html'
./Starfish 'test/cairo/internal-test/animation/a6.html'
./Starfish 'test/cairo/internal-test/animation/a7.html'
./Starfish 'test/cairo/internal-test/animation/a8.html'
./Starfish 'test/cairo/internal-test/animation/a9.html'
./Starfish 'test/cairo/internal-test/animation/a10.html'
./Starfish 'test/cairo/internal-test/animation/a11.html'
./Starfish 'test/cairo/internal-test/animation/a12.html'
./Starfish 'test/cairo/internal-test/animation/a13.html'
./Starfish 'test/cairo/internal-test/animation/a15.html'
./Starfish 'test/cairo/internal-test/animation/a16.html'
./Starfish 'test/cairo/internal-test/animation/a17.html'
./Starfish 'test/cairo/internal-test/animation/a18.html'
./Starfish 'test/cairo/internal-test/animation/a18-1.html'
./Starfish 'test/cairo/internal-test/animation/a18-2.html'
./Starfish 'test/cairo/internal-test/animation/a19.html'
./Starfish 'test/cairo/internal-test/animation/a20.html'
./Starfish 'test/cairo/internal-test/animation/a21.html'
./Starfish 'test/cairo/internal-test/animation/a22.html'
./Starfish 'test/cairo/internal-test/animation/a24.html'
./Starfish 'test/cairo/internal-test/animation/a25.html'
./Starfish 'test/cairo/internal-test/animation/a26.html'
./Starfish 'test/cairo/internal-test/animation/a28.html'
./Starfish 'test/cairo/internal-test/animation/a29.html'
./Starfish 'test/cairo/internal-test/animation/a30.html'
./Starfish 'test/cairo/internal-test/animation/a31.html'
./Starfish 'test/cairo/internal-test/animation/a32.html'
./Starfish 'test/cairo/internal-test/animation/a33.html'

# manual animation tc in wpt
./Starfish 'http://web-platform.test:8000/css/css-animations/animation-timing-function-007-manual.html'
./Starfish 'http://web-platform.test:8000/css/css-animations/animation-timing-function-009-manual.html'
./Starfish 'http://web-platform.test:8000/css/css-animations/animation-timing-function-010-manual.html'

# manual transition tc
./Starfish 'test/cairo/internal-test/transition/t10.html'
./Starfish 'test/cairo/internal-test/transition/t11.html'
./Starfish 'test/cairo/internal-test/transition/t12.html'
./Starfish 'test/cairo/internal-test/transition/t14.html'
./Starfish 'test/cairo/internal-test/transition/t15.html'

