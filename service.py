from aiohttp import web
import sys
import datetime

if len(sys.argv)<2:
	print("Port number is missing")
	exit()
if not sys.argv[1].isnumeric():
	print("Port number is not numeric")
	exit()
	
async def hello(request):
    return web.Response(text="Hello, world from "+sys.argv[1] + "\nLocal time is "+ str(datetime.datetime.now()))


app = web.Application()
app.add_routes([web.get('/', hello)])

web.run_app(app, port=int(sys.argv[1]))

