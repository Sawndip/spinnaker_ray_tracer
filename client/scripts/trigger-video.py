import cv, sys, pygame, math, time, copy
import listener, sender
from pygame.locals import *

position    = [-220.0, 50.0, 00.0]
look        = [1.0, 0.0, 0.0]
up          = [0.0, 1.0, 0.0]

moving      = 0;
strafing    = 0;
yawing      = 0;
pitching    = 0;
rolling     = 0;

FPS         = 60
moveAmount  = 40.0/FPS;
turnAmount  = 0.3/FPS;







# Functions for moving
def normalise(a):
    magnitude = (a[0]**2 + a[1]**2 + a[2]**2)**0.5
    return [a[0]/magnitude, a[1]/magnitude, a[2]/magnitude]

def cross_product(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

# Rotate the first vector about the second
def rotate(r1, r2, amount):
    c = math.cos(amount)
    s = math.sin(amount)
    t = 1.0 - c

    result = [0.0]*3
    result[0] = r1[0]*(t*r2[0]**2+c) + r1[1]*(t*r2[0]*r2[1]+s*r2[2]) + r1[2]*(t*r2[0]*r2[2]-s*r2[1])
    result[1] = r1[0]*(t*r2[0]*r2[1]-s*r2[2]) + r1[1]*(t*r2[1]**2+c) + r1[2]*(t*r2[1]*r2[2]+s*r2[0])
    result[2] = r1[0]*(t*r2[2]*r2[0]+s*r2[1]) + r1[1]*(t*r2[1]*r2[2]-s*r2[0]) + r1[2]*(t*r2[2]**2+c)

    return normalise(result)

def move(position, direction, amount):
    return [x+delta_x*amount for x,delta_x in zip(position, direction)]

def physics():
    global position, look, up

    # Move forwards/backwards
    position = move(position, look, moving*moveAmount)
    # Move left/right
    right = cross_product(look, up)
    position = move(position, right, strafing*moveAmount)
    # Pitch (Rotate the up and look vector about the right vector)
    look = rotate(look, right, pitching*turnAmount)
    up   = rotate(up,   right, pitching*turnAmount)
    # Yaw   (Rotate the look vector about the up vector)
    look = rotate(look, up, yawing*turnAmount)
    # Roll  (Rotate the up vector about the look vector)
    up = rotate(up, look, rolling*turnAmount)








def get_input():
    global moving,pitching,yawing,rolling,strafing
    for event in pygame.event.get():
        modifier = 0
        if event.type == KEYDOWN:
            modifier = 1
        if event.type == KEYDOWN or event.type == KEYUP:
            if event.key == ord('w'):
                moving = modifier
            elif event.key == ord('s'):
                moving = -modifier
            elif event.key == ord('a'):
                yawing = -modifier
            elif event.key == ord('d'):
                yawing = modifier
            elif event.key == ord('q'):
                strafing = -modifier
            elif event.key == ord('e'):
                strafing = modifier
            elif event.key == K_UP:
                pitching = modifier
            elif event.key == K_DOWN:
                pitching = -modifier
            elif event.key == K_LEFT:
                rolling = modifier
            elif event.key == K_RIGHT:
                rolling = -modifier








# Initialisation

horizontalFieldOfView   = 60.0
verticalFieldOfView     = 50.0
frameHeight             = 600

host = 'bluu'

if len(sys.argv) > 1:
    if sys.argv[1] == 'help':
        print "Usage:\npython drawer.py <host> <image height> <antialiasing> <x> <y> <p> <frameBuffering = 0,1>"
        print "\t-The antialiasing argument gives the number of rays cast per pixel in each dimension."
        print "\t-x and y are the dimensions of the grid of SpiNNaker chips."
        print "\t-p is the number of processors per chip."

        sys.exit()
    host = sys.argv[1]
if len(sys.argv) > 2:
    frameHeight = int(sys.argv[2])
frameWidth              = int(frameHeight * horizontalFieldOfView/verticalFieldOfView)
antialiasing            = 2
if len(sys.argv) > 3:
    antialiasing = int(sys.argv[3])
xChips = 1
yChips = 1
cores  = 16
if len(sys.argv) > 4:
    xChips = int(sys.argv[4])
if len(sys.argv) > 5:
    yChips = int(sys.argv[5])
if len(sys.argv) > 6:
    cores = int(sys.argv[6])
frameBuffering = True
if len(sys.argv) > 7:
    frameBuffering = bool(int(sys.argv[7]))
fps = 1
if len(sys.argv) > 8:
    fps = float(sys.argv[8])

sender.UDP_IP = host

activeFrame = 0
frame1 = cv.CreateMat(frameHeight, frameWidth, cv.CV_8UC3)
frame2 = cv.CreateMat(frameHeight, frameWidth, cv.CV_8UC3)
frames = [frame1, frame2]
pg_img = pygame.image.frombuffer(frames[activeFrame].tostring(), cv.GetSize(frames[activeFrame]), "RGB")

pygame.display.init()
screen = pygame.display.set_mode((10, 10), 0, 32)

screen.blit(pg_img, (0,0))
pygame.display.flip()

#The index is the global node ID, the value is [chip.x][chip.y][processor]
nodes = [] # nodes that we think are alive
deadNodes = [] # nodes that we know are dead
activeNodes = [] # nodes that have sent a 'finish' message for the most recent frame
lastReceiveTime = [time.time()]
diff = lambda l1,l2: [x for x in l1 if x not in l2] # for getting items that are in deadNodes but not nodes
'''if frameBuffering:
    listener.setup(frame2, frameWidth, frameHeight, nodes, activeNodes, lastReceiveTime, host, 44444) 
else:
    listener.setup(frame1, frameWidth, frameHeight, nodes, activeNodes, lastReceiveTime, host, 44444) '''

# Establish which nodes are alive
for x in range(xChips):
    for y in range(yChips):
        for p in range(1, cores+1, 1):
            nodes.append([x,y,p])
nodes.remove([0,0,1]) # This node is the comms aggregator

# End of initialisation









def update_image():
    global activeNodes
    for i in range(len(activeNodes)):
        activeNodes.pop()
    #camx, camy, camz, lookx, looky, lookz, upx, upy, upz, width, height, hField, vField, antialiasing, chip.x, chip.y, processor, global node ID, number of nodes

    for i in range(len(nodes)):
        sender.send_trace_trigger(int(position[0]*2**9),int(position[1]*2**9),int(position[2]*2**9),int(look[0]*2**16),int(look[1]*2**16),int(look[2]*2**16),int(up[0]*2**16),int(up[1]*2**16),int(up[2]*2**16),frameWidth,frameHeight,int(horizontalFieldOfView*2**16),int(verticalFieldOfView*2**16),antialiasing, nodes[i][0], nodes[i][1], nodes[i][2], i, len(nodes))
time.sleep(0.01)

update_image()

prev = time.time()

print "Attempting to run ray tracer on %dx%d chips using %d cores in each." % (xChips, yChips, cores)
print "Node <0,0,1> is the communications aggregator."
print "Attempting to run on %d cores.\n" % (xChips*yChips*cores-1)

clock = pygame.time.Clock()

frameTimes = []
lastFrameTime = time.time()
tick = 0
while not pygame.QUIT in [e.type for e in pygame.event.get()]:
    clock.tick(FPS) # wait 1/30th of a second

    physics() # Calculate movement

    if time.time() - prev > (1/fps):
        prev=time.time()
        update_image()

    get_input()
    pg_img = pygame.image.frombuffer(frames[activeFrame].tostring(), cv.GetSize(frames[activeFrame]), "RGB")
    #screen.blit(pg_img, (0,0))
    #pygame.display.flip()  

    # Last receive time is the time of receipt of last 'finish' packet
    # If the time spent on this frame is more than double last receive time minus frame start time, consider the nodes that haven't responded to be lost
    if (time.time()-lastFrameTime) / (lastReceiveTime[0]-lastFrameTime) > 3.0 and len(activeNodes)>len(nodes)/2:
        for i in diff(nodes, activeNodes):
            #print "Chip", i[0], i[1], "processor", i[2], "has stopped responding.\n"
            nodes.remove(i)

    # If we've got a full frame
    if len(activeNodes) == len(nodes):
        
        #listener.setFrame(frames[activeFrame])
        if frameBuffering:
            activeFrame = 1 - activeFrame
        update_image()
        frameTimes.insert(0, time.time())
        if len(frameTimes)>5:
            frameTimes.pop()
        lastFrameTime = time.time()

    if tick%10==9:
        if len(frameTimes)>1:
            print "FPS:", "{0:.2f}".format(len(frameTimes)/(frameTimes[0]-frameTimes[len(frameTimes)-1])), "  Active nodes: %d/%d                \r" % (len(nodes), xChips*yChips*cores-1),
            sys.stdout.flush()
        if time.time() - lastFrameTime > 5.0:
            print "FPS: 0.00   Active nodes: %d/%d                \r" % (len(nodes), xChips*yChips*cores-1),
            sys.stdout.flush()
    tick+=1


