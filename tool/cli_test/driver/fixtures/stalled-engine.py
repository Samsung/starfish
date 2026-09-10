#!/usr/bin/env python3

import os
import socket
import time


server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", int(os.environ["STARFISH_CDP_PORT"])))
server.listen()
connection, _ = server.accept()
time.sleep(60)
