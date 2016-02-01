import sys,time,math,numpy,socket,string,struct

UDP_IP='bluu'
SPINN_PORT = 17893

def setup(host):
    global UDP_IP
    UDP_IP=host

s = None

tx_spinn_header = {
    'reserved'          : 0,
    'flags'             : 0x07, 
    'ip_tag'            : 0,
    'destination_port'  : 1<<5 | 1,
    'source_port'       : 0,
    'destination_chip'  : 0,
    'source_chip'       : 0,
    'command'           : 0,
    'sequence'          : 0,
    'arg1'              : 0,
    'arg2'              : 0,
    'arg3'              : 0
}

def pack_spinn_header():
    '''Returns headers of SpiNNaker packet packed into a string for
       transmission over Ethernet. The endianness argument determines whether
       the data is packed in host or network order. It must be set to True for
       system software loading (the boot ROM code performs hardware ntoh).'''
    format = '=HBBBBHHHHIII'
    return struct.pack(format,
                        tx_spinn_header['reserved'],
                        tx_spinn_header['flags'],
                        tx_spinn_header['ip_tag'],
                        tx_spinn_header['destination_port'],
                        tx_spinn_header['source_port'],
                        tx_spinn_header['destination_chip'],
                        tx_spinn_header['source_chip'],
                        tx_spinn_header['command'],
                        tx_spinn_header['sequence'],
                        tx_spinn_header['arg1'],
                        tx_spinn_header['arg2'],
                        tx_spinn_header['arg3'],)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def send_trace_trigger(camx, camy, camz, lookx, looky, lookz, upx, upy, upz, width, height, hField, vField, antialiasing, x, y, processor, globalNodeID, numberOfNodes):
    global tx_spinn_header

    tx_spinn_header['destination_port'] = ((1<<5) | processor)
    tx_spinn_header['destination_chip'] = (x<<8) + y
    tx_spinn_header['command'] = 4

    toSend = pack_spinn_header()

    # camera variables and field of view variables need to be scaled up << 16
    toSend += struct.pack("=iiiiiiiiiiiiiiii", camx, camy, camz, lookx, looky, lookz, upx, upy, upz, width, height, hField, vField, antialiasing, globalNodeID, numberOfNodes)

    sock.sendto(toSend, (UDP_IP, SPINN_PORT))


