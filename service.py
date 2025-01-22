from aiohttp import web
import sys

if len(sys.argv)<2:
	print("Port number is missing")
	exit()

async def hello(request):
    return web.Response(text="Hello, world from "+sys.argv[1])


app = web.Application()
app.add_routes([web.get('/', hello)])

web.run_app(app, port=int(sys.argv[1]))

