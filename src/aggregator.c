#include "spin1_api.h"
#include "spinn_io.h"
#include "spinn_sdp.h"

#define SDP_HEADER_SIZE             (24) // bytes

uint lower16 = (1<<16)-1;
uint upper16 = ~((int)((1<<16)-1));
uint lower8  = (1<<8)-1;
uint upper8  = ~((int)((1<<8)-1));

const uint firstByte  = (1<<8)-1;
const uint secondByte = (firstByte<<8);
const uint thirdByte  = (secondByte<<8);
const uint fourthByte = (thirdByte<<8);

sdp_msg_t message;
int pixel;

void sdp_packet_callback(uint msg, uint port)
{
    sdp_msg_t *sdp_msg = (sdp_msg_t *) msg;

    // Unimplemented

    spin1_msg_free(sdp_msg);
}

int mc_rx  = 0;
int sdp_tx = 0;
int successful_sdp_tx = 0;

void mc_packet_callback(uint key, uint payload)
{
    mc_rx += 1;

    message.data[pixel*7]   = key&firstByte;                // x1
    message.data[pixel*7+1] = ((payload&fourthByte)>>24);   // x2
    message.data[pixel*7+2] = ((key&fourthByte)>>24);       // y1
    message.data[pixel*7+3] = ((key&thirdByte)>>16);        // y2
    message.data[pixel*7+4] = ((payload&thirdByte)>>16);    // r
    message.data[pixel*7+5] = ((payload&secondByte)>>8);    // g
    message.data[pixel*7+6] = payload&firstByte;            // b

    pixel += 1;

    if (pixel == 36) // the payload of SDP packet is full
    {
        io_printf(IO_STD, "Sending SDP packet.\n");
        message.length = SDP_HEADER_SIZE + pixel*7;
        if (spin1_send_sdp_msg(&message, 1))
            successful_sdp_tx += 1;
        message.arg1   = pixel;
            

        pixel = 0;
        sdp_tx += 1;
    }
}

void timer_callback(uint time, uint none)
{
    io_printf(IO_STD, "Received %d MC packets.  Sent %d SDP packets, %d successful.\n", mc_rx, sdp_tx, successful_sdp_tx);
}

void c_main()
{   
    io_printf(IO_STD, "Started aggregator.\n");
    io_printf(IO_STD, "Started aggregator.\n");
    io_printf(IO_STD, "Started aggregator.\n");
    io_printf(IO_STD, "Started aggregator.\n");
    io_printf(IO_STD, "Started aggregator.\n");

    message.flags          = 0x07;
    message.tag            = 1;
    message.dest_port      = PORT_ETH;
    message.srce_port      = (1<<5) | spin1_get_core_id();
    message.dest_addr      = 0;
    message.srce_addr      = spin1_get_chip_id();
    message.cmd_rc         = 3; // Pixel message*/
    pixel = 0;

    spin1_set_timer_tick(1000000);
    //spin1_callback_on(TIMER_TICK, timer_callback, 3);
    spin1_callback_on(SDP_PACKET_RX, sdp_packet_callback, 2);
    spin1_callback_on(MC_PACKET_RECEIVED, mc_packet_callback, 1);

    spin1_start();
}
