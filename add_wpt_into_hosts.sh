#!/bin/bash
PATTERN=web-platform.test
FILE=/etc/hosts
if grep -q $PATTERN $FILE;
 then
    exit 0
fi

cat <<EOF >> /etc/hosts
127.0.0.1	web-platform.test
127.0.0.1	www.web-platform.test
127.0.0.1	www1.web-platform.test
127.0.0.1	www2.web-platform.test
127.0.0.1	xn--n8j6ds53lwwkrqhv28a.web-platform.test
127.0.0.1	xn--lve-6lad.web-platform.test
0.0.0.0	nonexistent-origin.web-platform.test
EOF

