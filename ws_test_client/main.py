import websocket
ws = websocket.WebSocket()
ws.connect("ws://localhost:8080/")
ws.send("{\"type\": \"register\", \"username\": \"oipo\", \"password\": \"test\", \"email\": \"test@test.nl\"}")
res = ws.recv()
print(f'res: {res}')
