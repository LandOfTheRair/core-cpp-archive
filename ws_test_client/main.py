import websocket
import ssl
import json
import sys

ws = websocket.WebSocket(sslopt={"cert_reqs": ssl.CERT_NONE})
ws.connect("wss://localhost:8080/")
#ws.connect("wss://62.210.141.213:8080/")
ws.send("{\"type\": \"Auth:register\", \"username\": \"oipo\", \"password\": \"test\", \"email\": \"test@test.nl\"}")
res = ws.recv()
print(f'res: {res}')
res_msg = json.loads(res)
if res_msg['type'] == 'error_response':
    ws.send("{\"type\": \"Auth:login\", \"username\": \"oipo\", \"password\": \"test\"}")
    res = ws.recv()
    print(f'res: {res}')
    res_msg = json.loads(res)
    if res_msg['type'] == 'error_response':
        sys.exit(1)


ws.send("{\"type\": \"Game:create_character\", \"name\": \"oipo3\", \"sex\": \"test\", \"allegiance\": \"test\", \"baseclass\": \"baseclass\"}")
res = ws.recv()
print(f'res: {res}')
ws.send("{\"type\": \"Game:play_character\", \"name\": \"oipo3\"}")
res = ws.recv()
print(f'res: {res}')
res_msg = json.loads(res)
if res_msg['type'] == 'error_response':
    sys.exit(1)

ws.send("{\"type\": \"Moderator:motd\", \"motd\": \"Welcome to rair!\"}")
ws.send("{\"type\": \"Game:move\", \"x\": 12, \"y\": 12}")
ws.send("{\"type\": \"Chat:send\", \"content\": \"chat message!\"}")
while 1:
    res = ws.recv()
    print(f'res: {res}')
