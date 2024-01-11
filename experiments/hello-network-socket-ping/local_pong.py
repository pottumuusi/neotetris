"""
pong.py

BUG: As far as I know, the listen socket does not get closed.
"""

import socket
import time

g_DEBUG = False

"""
On each call, a new socket is created and then left unattended upon return.
"""
def listen_for_connection():
    host = '' # Empty string means all interfaces and causes listen(?)
    port = 10001
    address = None
    sock_listen = None
    unaccept_backlog = 3

    sock_listen = socket.socket()
    sock_listen.bind( (host, port) )
    print(f'Socket bound to port {port}')

    sock_listen.listen(unaccept_backlog)
    print('Socket listening')

    print('Waiting to accept a connection')
    sock_connected, address = sock_listen.accept()
    print(f'Accepted a connection from address: {address}')

    return sock_connected

def read_from_connection(sock_connected):
    buffer = 4096
    data_received = None

    data_received = sock_connected.recv(buffer)

    return data_received

def is_ping(data_received):
    expected_str = 'ping'

    str_received = data_received.decode('utf-8')

    if (g_DEBUG):
        print(f'data_received is: {data_received}')
        print(f'str_received is: {str_received}')

    if (expected_str == str_received):
        return True

    return False

def send_pong(sock_connected):
    message_pong = b'pong'

    sock_connected.send(message_pong)

if __name__ == '__main__':
    print('Pong starting up...')

    while True:
        sock_connected = listen_for_connection()
        data_received = read_from_connection(sock_connected)

        if (is_ping(data_received)):
            print('Received ping!')

            send_pong(sock_connected)
            print('Sent pong')

            if (g_DEBUG):
                time.sleep(0.1)

            sock_connected.close()
            print('Closed socket')
            break
