import socket

client = socket.socket()
client.connect(('127.0.0.1',8494))

def parse (msg):
    parsed = eval(f'"{msg}"')
    return parsed.replace('/', '\n').encode()
try:
    while True:
        request = input()
        if request != '':
            client.send(parse(request))

        print(client.recv(1024))
except KeyboardInterrupt:
    ''
