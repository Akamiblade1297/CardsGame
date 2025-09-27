import socket

client = socket.socket()
client.connect(('127.0.0.1',8494))

try:
    while True:
        request = input().replace('/','\n')
        if request != '':
            client.send(request.encode())

        response = client.recv(1024)
        print(f"Server: {response.decode()}\n")
except KeyboardInterrupt:
    ''
