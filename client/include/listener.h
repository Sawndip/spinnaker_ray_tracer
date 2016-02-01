int frameWidth, frameHeight, antialiasing;
unsigned char *viewingFrame, *drawingFrame;
int frameBuffering;
clock_t lastMessageTime;

void* input_thread (void *ptr);

void send_trace_trigger(int camx, int camy, int camz, int lookx, int looky, int lookz, int upx, int upy, int upz, int width, int height, int hField, int vField, int antialiasing, int x, int y, int processor, int globalNodeID, int numberOfNodes);
