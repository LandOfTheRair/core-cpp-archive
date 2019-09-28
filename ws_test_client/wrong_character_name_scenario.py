import websocket
import ssl
import json
import sys

ws = websocket.WebSocket(sslopt={"cert_reqs": ssl.CERT_NONE})
ws.connect("wss://localhost:8080/")
#ws.connect("wss://62.210.141.213:8080/")
ws.send("{\"type\": \"Auth:register\", \"username\": \"wrongname\", \"password\": \"testtest\", \"email\": \"test@test.nl\"}")
res = ws.recv()
print(f'res: {res}')
res_msg = json.loads(res)
if res_msg['type'] == 'error_response':
    ws.send("{\"type\": \"Auth:login\", \"username\": \"wrongname\", \"password\": \"testtest\"}")
    res = ws.recv()
    print(f'res: {res}')
    res_msg = json.loads(res)
    if res_msg['type'] == 'error_response':
        sys.exit(1)


ws.send("{\"type\": \"Game:create_character\", \"slot\": 0, \"name\": \"漢\", \"gender\": \"test\", \"allegiance\": \"Pirates\", \"baseclass\": \"Mage\"}")
res = ws.recv()
print(f'res: {res}')
ws.send("{\"type\": \"Game:create_character\", \"slot\": 0, \"name\": \"o1po\", \"gender\": \"test\", \"allegiance\": \"Pirates\", \"baseclass\": \"Mage\"}")
res = ws.recv()
print(f'res: {res}')
ws.send("{\"type\": \"Game:create_character\", \"slot\": 0, \"name\": \"oi po\", \"gender\": \"test\", \"allegiance\": \"Pirates\", \"baseclass\": \"Mage\"}")
res = ws.recv()
print(f'res: {res}')
ws.send("{\"type\": \"Game:create_character\", \"slot\": 0, \"name\": \"oipo\", \"gender\": \"test\", \"allegiance\": \"Pirates\", \"baseclass\": \"Mage\"}")
res = ws.recv()
print(f'res: {res}')
ws.send("{\"type\": \"Game:create_character\", \"slot\": 0, \"name\": \"бipб\", \"gender\": \"test\", \"allegiance\": \"Pirates\", \"baseclass\": \"Mage\"}")
res = ws.recv()
print(f'res: {res}')
