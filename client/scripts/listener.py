#!/usr/bin/python

import socket, struct, sys, threading, time, sender

image = None
width = height = 100

lower = (2**8)-1
upper = ~lower

receiver = None
drawer   = None

class Drawer ( threading.Thread ):
    
    def initialise(self):
        self.drawing = 0
        self.pixelPacketBuffer = []
        self.iteration = 0

    def run(self):
        while True:
            if len(self.pixelPacketBuffer) > 0:
                packet = self.pixelPacketBuffer.pop()

                for i in range(len(packet)/7):
                    x = (ord(packet[7*i])<<8) + ord(packet[7*i+1])
                    y = (ord(packet[7*i+2])<<8) + ord(packet[7*i+3])

                    image[y,x] = (ord(packet[7*i+4]), ord(packet[7*i+5]), ord(packet[7*i+6]))

class Receiver( threading.Thread ):

    def initialise(self, sock, address, nodes, activeNodes, lastReceiveTime):
        self.running = True

        self.drawers = [Drawer(),Drawer(),Drawer()]
        for drawer in self.drawers:
            drawer.initialise()
            drawer.start()
        self.drawerIndex = 0
        
        self.sock = sock
        self.sock.settimeout(1)
        
        self.address = address
    
        self.nodes = nodes
        self.activeNodes = activeNodes
        self.lastReceiveTime = lastReceiveTime

        self.SDP_HEADER_FORMAT = '=HBBBBHHHHIII'
        self.SDP_HEADER_SIZE = struct.calcsize(self.SDP_HEADER_FORMAT)

        self.routing_keys = list()

    def log(self, data):
        global image

        if len(data) >= self.SDP_HEADER_SIZE:
            sdp_message = self.unpack_sdp_message(data)
            '''print "packet length", len(data)
            print "flags ", sdp_message['flags']
            print "tag ", sdp_message['ip_tag']
            print "dest port ", sdp_message['destination_port']
            print "source port ", sdp_message['source_port']
            print "destination chip ", sdp_message['destination_chip']
            print "source chip ", sdp_message['source_chip']
            print "command ", sdp_message['command']    
            print "arg1 ", sdp_message['arg1']
            print "arg2 ", sdp_message['arg2']
            print "arg3 ", sdp_message['arg3']
            print "length of payload ", len(sdp_message['payload'])
            for i in sdp_message['payload']:
                print ord(i),
            print "\n\n"'''

            if sdp_message['command'] == 3: # pixels
                self.drawers[self.drawerIndex].pixelPacketBuffer.append(sdp_message['payload'])
                self.drawerIndex = (self.drawerIndex + 1) % len(self.drawers)

                '''for i in range(len(sdp_message['payload'])/7):
                    x = (ord(sdp_message['payload'][7*i])<<8) + ord(sdp_message['payload'][7*i+1])
                    y = (ord(sdp_message['payload'][7*i+2])<<8) + ord(sdp_message['payload'][7*i+3])

                    image[y,x] = (ord(sdp_message['payload'][7*i+4]), ord(sdp_message['payload'][7*i+5]), ord(sdp_message['payload'][7*i+6]))'''

            elif sdp_message['command'] == 7: # an "I've finished tracing" message
                x = ((sdp_message['source_chip']&upper)>>8)
                y = (sdp_message['source_chip']&lower)
                proc = sdp_message['arg2']
                if [x,y,proc] not in self.activeNodes:
                    self.activeNodes.append([x,y,proc])
                self.lastReceiveTime[0] = time.time()
                
                

    def run(self):
        self.sock.bind(('0.0.0.0', self.address[1])) # Bind all addresses on given port
        
        while self.running:
            try:
                data = self.sock.recv(1024)
                self.log(data)
            
            except socket.timeout:
                pass


    def unpack_sdp_message(self, data):
        sdp_message = {
            'reserved': 0,
            'flags': 0,
            'ip_tag': 0,

            'destination_port': 0,
            'source_port': 0,
            'destination_chip': 0,
            'source_chip': 0,

            'command': 0,
            'sequence': 0,
            'arg1': 0,
            'arg2': 0,
            'arg3': 0,

            'payload': []
        }

        unpack = struct.unpack(self.SDP_HEADER_FORMAT, data[:self.SDP_HEADER_SIZE])
        sdp_message['reserved'] = unpack[0]
        sdp_message['flags'] = unpack[1]
        sdp_message['ip_tag'] = unpack[2]
        sdp_message['destination_port'] = unpack[3]
        sdp_message['source_port'] = unpack[4]
        sdp_message['destination_chip'] = unpack[5]
        sdp_message['source_chip'] = unpack[6]
        sdp_message['command'] = unpack[7]
        sdp_message['sequence'] = unpack[8]
        sdp_message['arg1'] = unpack[9]
        sdp_message['arg2'] = unpack[10]
        sdp_message['arg3'] = unpack[11]
        sdp_message['payload'] = data[self.SDP_HEADER_SIZE:]

        return sdp_message

def setFrame(frame):
        global image
        image = frame

def setup(frame, frameWidth, frameHeight, nodes, activeNodes, lastReceiveTime, host, port):

    global image, width, height, receiver

    image = frame
    width = frameWidth
    height = frameHeight
   
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    address = (host, port)
    
    try:
        receiver = Receiver()
        receiver.initialise(sock, address, nodes, activeNodes, lastReceiveTime)
        receiver.start()



    except KeyboardInterrupt:
        receiver.running = False
