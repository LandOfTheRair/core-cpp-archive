import websocket
import json
import sys
ws = websocket.WebSocket()
ws.connect("ws://localhost:8080/")
ws.send("{\"type\": \"Auth:register\", \"username\": \"oipo\", \"password\": \"test\", \"email\": \"test@test.nl\"}")
res = ws.recv()
print(f'res: {res}')
res_msg = json.loads(res)
if res_msg['type'] == 'error_response':
    ws.send("{\"type\": \"Auth:login\", \"username\": \"oipo\", \"password\": \"test\", \"email\": \"test@test.nl\"}")
    res = ws.recv()
    print(f'res: {res}')
    res_msg = json.loads(res)
    if res_msg['type'] == 'error_response':
        sys.exit(1)


ws.send("{\"type\": \"Game:create_character\", \"name\": \"oipo\", \"sex\": \"test\", \"allegiance\": \"test\", \"baseclass\": \"baseclass\"}")
res = ws.recv()
print(f'res: {res}')
res_msg = json.loads(res)
if res_msg['type'] == 'error_response':
    ws.send("{\"type\": \"Game:play_character\", \"name\": \"oipo\"}")
    res = ws.recv()
    print(f'res: {res}')
    res_msg = json.loads(res)
    if res_msg['type'] == 'error_response':
        sys.exit(1)
while 1:
    res = ws.recv()
    print(f'res: {res}')
