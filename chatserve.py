#
#	August Lautt
#
#	Description:
#	Server-side TCP chat application using socket api to bind connection, listen
#	for clients and then transmit messages back and forth with a client.
#	Additional information available in the readme.
#	Usage:
#	chatserve.py [portnumber]
#

import socket
import sys


#
#	receiveMsg(): Receives incoming data and either print incoming message or
#	prints that the client has disconnected.
#
#	Takes connection socket object created when connection accepted
#	to use with the recv method.
#	Returns True to continue chat with current connection or False to stop.
#
def receiveMsg(conn):
	# receive the message up to 513 character including username, "> ", "\0"
	message = conn.recv(513)

	# check if client has quit in their string and return the flag
	if message.find('\quit') != -1:
		print 'Client disconnected from server'
		return False

	# otherwise, print the received message and return the flag
	else:
		print(message)

		return True

#
#	sendMsg(): Prompts user for message text, then either sends data or sends
#	quit data and flags the program to quit.
#
#	Takes server handle and connection socket object created when connection
#	accepted to use with the send method.
#	Returns True to continue chat with current connection or False to stop.
#
def sendMsg(conn, handle):
	# prompt and get user input
	message = raw_input(handle + '> ')

	# check if user wants to quit and return the flag
	if message == '\\quit':
		print 'Terminating connection'
		return False

	# otherwise, prepend the message and truncate at 513
	else:
		outgoing = handle + '> ' + message
		outgoing = outgoing[:513]

		# send the message using socket connection object and return flag
		conn.send(outgoing)
		return True

#
#	createConn(): Prompts user for message text, then either sends data or sends
#	quit data and flags the program to quit.
#
#	Takes the port number supplied in the arguments.
#	Returns True to continue chat with current connection or False to stop.
#
def createConn(portNum):
	hostName = socket.gethostname()

	# create socket with TCP and IPv4/IPv6 set
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	# bind the socket to the address with the hostname and provided port number
	sock.bind((hostName, portNum))

	# listen for one connection on the newly created socket
	sock.listen(1)

	# return the socket
	return sock


def main(argv):
	if len(sys.argv) != 2:
		print('Usage: chatserve.py [hostname]')
		quit()

	print 'Welcome to the chat server!'

	# convert the port number argument to int
	port = int(sys.argv[1])

	# create a socket and begin listening for a connection
	sock = createConn(port)
	handle = 'SERVERNAME'

	# loop forever, only exit out with SIGINT
	while True:
		print 'The server is now listening for connections'

		# Create connection socket object and accept connection
		conn, address = sock.accept()

		# loop receiving and sending while neither server or client has quit
		chatting = True
		while chatting:
			chatting = receiveMsg(conn)
			if chatting:
				chatting = sendMsg(conn, handle)

		# close current connection and loop to listen for another
		conn.close()


main(sys.argv)