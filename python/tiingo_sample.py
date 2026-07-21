#!/usr/bin/python


from websocket import create_connection
import simplejson as json
ws = create_connection("wss://api.tiingo.com/iex")

subscribe = {
        'eventName':'subscribe',
        'authorization':'81a156657568df93ed903cbc577872f2c8bc70dd',
        'eventData': {
            'thresholdLevel': 5,
            'tickers': ['aapl', 'uso']
    }
}

print(json.dumps(subscribe))

ws.send(json.dumps(subscribe))
while True:
    print(ws.recv())
