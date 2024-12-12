#!/usr/bin/env python3

import os
import sys
import signal
import socket
import threading
from time import sleep


def main(argv):
	'''! Program anchor

	@param  argc The number of arguments to the program
	@param  argv The arguments passed to the program

	@return      The program exit code
	'''

	addr, port = parseArgs(argv)
	connect(addr, port)

	reqLoadFile("cubes.clp");
	reqReset();
	reqRun();

	print('\n\nCool, right!? Let\'s retry adding another cube...', end='')
	reqReset()
	reqAssert("(block G)")
	reqAssert("(on-top-of (upper nothing)(lower G))")
	reqAssert("(on-top-of (upper G)(lower D))")
	reqRun()

	print('\n\nNow with yet another cube...')
	reqReset()
	print('\nUpon reset, we lost G, so let\'s add it again...', end='')
	reqAssert("(block G)")
	reqAssert("(on-top-of (upper nothing)(lower G))")
	reqAssert("(on-top-of (upper G)(lower D))")
	print('\nNow with H...', end='')
	reqAssert("(block H)")
	reqAssert("(on-top-of (upper nothing)(lower H))")
	reqAssert("(on-top-of (upper H)(lower A))")
	reqRun()

	print('All done. Shutting down... ')

	cleanup()
	print('Bye!\n')
	sys.exit(0)
#end def


# ## #########################################################
# Function definitions
# ## #########################################################

def reqLoadFile(file:str):
	'''!Requests clips to load the specified file

	@param file Path to file to load
	'''

	print(f'Press enter to load { file }', end='')
	input()
	publish( "(clear)" )
	sleep( 0.25 )
	publish( "(load cubes.clp)" )
#end def


def reqReset():
	'''!Requests clips to reset the KDB
	'''

	print('Press enter to reset CLIPS', end='')
	input()
	publish( "(reset)" )
#end def


def reqRun():
	'''!Requests clips to run the KDB engine
	'''

	print('Press enter to run CLIPS', end='')
	input()
	publish( "(run -1)" )
	sleep(1)
#end def


def reqAssert(fact:str):
	'''!Requests clips to assert a fact.
	'''

	print(f'Press enter to assert {fact}', end='')
	input()
	publish( "(assert " + fact + ")" )
#end def


def reqRetract(fact:str):
	'''!Requests clips to retract a fact.
	'''

	print(f'Press enter to retract {fact}', end='')
	input()
	publish( "(retract " + fact + ")" )
#end def


def publish(s:str):
	'''!Sends the given strng to clips

	@param s The string to send
	'''
	msg = f'\0raw {s}\n'
	payload = msg.encode('utf8')
	sckt.send( payload )
#end def


def parseArgs(argv):
	return 'localhost', 5000
#end def


def connect(addr, port):
	'''!Establishes a TCP connection with CLIPS
	'''

	global sckt, running, thread
	sckt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		sckt.connect( (addr, port) )
	except:
		print(f'Could not connect to CLIPS on {addr}:{port}.')
		print('Run the server and pass the right parameters.')
		sys.exit(-1)

	running = True
	thread = threading.Thread(target=async_rcv_loop)
	thread.start()
#end def


def async_rcv_loop():
	while running:
		data = sckt.recv(65536)
		if data == b'':
			print('\nDisconnected')
			sckt.shutdown(0)
			os.kill(os.getpid(), signal.SIGINT)
			return

		msg = data.decode('utf8')
		if msg:
			print(f'CLIPS: { msg.strip() }')
#end def


def cleanup():
	global running
	running = False
	try:
		sckt.shutdown(0)
		sckt.close()
	except: pass
	thread.join(500)
#end def


if __name__ == '__main__':
	main(sys.argv)
