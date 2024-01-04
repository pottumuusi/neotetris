"""
ping.py
"""

import socket

g_DEBUG = False
g_socket = None

def send_ping():
    global g_socket

    server_host = '127.0.0.1'
    server_port = 10001

    message_ping = b'ping'

    g_socket = socket.socket()

    g_socket.connect( (server_host, server_port) )
    print(f'Connected to {server_host} port {server_port}')

    g_socket.send(message_ping)
    print('Ping sent')

"""
Expecting socket to be connected before calling this function.
"""
def read_from_connected_socket():
    buffer = 4096

    data_received = g_socket.recv(buffer)

    return data_received

def is_pong(data_received):
    expected_str = 'pong'

    str_received = data_received.decode('utf-8')

    if (expected_str == str_received):
        return True

    return False

def finish_communication(data_received):
    global g_socket

    if (is_pong(data_received)):
        print('Successfully received pong!')
        g_socket.close()
        print('Closed socket')
        return

    print('No pong received.')
    g_socket.close()
    print('Closed socket')

if __name__ == '__main__':
    print('Ping starting up...')

    if (g_DEBUG):
        print(f'g_socket before ping send is: {g_socket}')
    send_ping()
    if (g_DEBUG):
        print(f'g_socket after ping send is: {g_socket}')

    data_received = read_from_connected_socket()

    if (g_DEBUG):
        print(f'data_received is: {data_received}')

    finish_communication(data_received)
