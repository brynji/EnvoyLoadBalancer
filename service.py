from aiohttp import web
import sys
import datetime
import time

if len(sys.argv)<2:
	print("Port number is missing")
	exit()
if not sys.argv[1].isnumeric():
	print("Port number is not numeric")
	exit()
	
async def hello(request):
    print("Got request!")
    time.sleep(2)
    return web.Response(text="Hello, world from "+sys.argv[1] + "\nLocal time is "+ str(datetime.datetime.now())+ "\nRoute " + request.path)


app = web.Application()
app.router.add_route('*', '/{tail:.*}', hello)

web.run_app(app, port=int(sys.argv[1]))

