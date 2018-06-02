# include "Python.h"
#include <stdint.h>
#include "deps/pylvgl/lvgl/lvgl.h"
#include "hal.h"
#include "bcm2835_dma.h"
#include "bcm2835_spi.h"


static bcm2835_dma_conblk rx_conblk[1];
static bcm2835_dma_conblk tx_conblk[21];

static uint32_t tft_pinmask;
static uint32_t tft_cmds[] = {
    0x000100B0, // Transfer length 1
    0x0000002A, // Command 0x2A
    0x000400B0, // Transfer length 4
    0x3f010000, // start column, end column (bytes swapped)
    0x000100B0, // Transfer length 1
    0x0000002B, // Command 0x2B
    0x000400B0, // Transfer length 4
    0xef000000, // start row, end row (bytes swapped)
    0x000100B0, // Transfer length 1
    0x0000002C, // Command 0x2C
    0x7d0000B0, // Transfer length 32000
    0x00000001, // Used to start rxdma channel

};

static char swapbuffer[32000];


static inline unsigned int swapbytes(unsigned int x) {
    return ((x<<8)&0xff00) | ((x>>8) & 0xff);
}

static void pitft_transfer_done(SPIDriver *spip) {
    lv_flush_ready();
}

/*
 * The pitft_update function is completely DMA driven. This function builds
 * two DMA-chains, rx_conblk and tx_conblk. Most work is done in the
 * tx_conblk chain. It does the following:
 *
 * 1) Swap data bytes of the framebuffer (switch endianess)
 *    a) Copy all framebuffer bytes to swapbuffer, shifted one byte back
 *    b) Copy every even byte, shifted one byte forward
 *
 * 2) Send command 0x2A (Column address set):
 *    a) GPIO25 low (Data/nCommand)
 *    b) Write 5 bytes to SPI FIFO register:
 *       i)  1 word: data length (=1) + SPI config
 *       ii) 1 byte command 0x2A
 *    c) Read 1 byte from SPI FIFO register
 *    d) GPIO25 high
 *    e) Write 8 bytes to SPI FIFO register:
 *       i)  1 word: data length (=4) + SPI config
 *       ii) 1 word data (column start + end address in big-endian order)
 *    f) Read 4 bytes from SPI FIFO register
 *
 * 3) Send command 0x2B (Row address set):
 *    - same procedure as 2)
 *
 * 4) Send command 0x2C (send data):
 *    - same procedure as 2), but skip steps e) and f)
 *
 * 5) Write 4 bytes to SPI FIFO register:
 *    - 1 word: data length + SPI config
 *
 * 6) Start the rx_conblk DMA chain
 *
 * 7) Send the data bytes
 *
 * While the tx_conblk is sending the data bytes (step 7), the rx_conblk DMA
 * chain reads and discards the received data.
 *
 */
void pitft_update(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * data) {
    bcm2835_dma_channel_t rx_dma_channel;
    bcm2835_dma_channel_t tx_dma_channel;
    bcm2835_dma_conblk *blk = tx_conblk;
    unsigned long nbytes;

    nbytes = 2* ((x2-x1)+1)* ((y2-y1)+1);

    if (nbytes > 0x7ff0) {
        return; // Too large transfer, not supported
    }
    
    spi_lld_get_dma_channels(&rx_dma_channel, &tx_dma_channel);

    printf("%ld %ld %ld %ld %d %d\n", x1,y1,x2,y2,rx_dma_channel, tx_dma_channel);
    
    tft_cmds[3] = (swapbytes(x2) << 16) | swapbytes(x1);
    tft_cmds[7] = (swapbytes(y2) << 16) | swapbytes(y1);
    tft_cmds[10] = (nbytes << 16) | 0xb0;

    tft_pinmask = 1<<25;
    
    
    bcm2835_dma_fill_conblk(blk++, swapbuffer, ((char*)data) + 1, nbytes-1, 0); // 1a) copy framebuffer+1 to swapbuffer
    bcm2835_dma_fill_conblk_2d(blk++, swapbuffer+1, data, 1, nbytes >> 1, 1, 1, 0); // 1b) copy even bytes of framebuffer to swapbuffer + 1
    
    for(int i = 0; i<3;i++) {
    
        bcm2835_dma_fill_conblk(blk++, (void*) &(GPCLR0), &tft_pinmask, 4, 0); //2a)
        
        bcm2835_dma_fill_conblk(blk++, (void*) &(BCM2835_SPI->fifo), &tft_cmds[4*i], 5, 6); //2b) send transfer length + 1 byte
        bcm2835_dma_fill_conblk(blk++, NULL, (void*) &(BCM2835_SPI->fifo), 1, 7); // 2c) read 1 byte
        
        bcm2835_dma_fill_conblk(blk++, (void*) &(GPSET0), &tft_pinmask, 4, 0); // 2d)
                
        if (i<2) {
            bcm2835_dma_fill_conblk(blk++, (void*) &(BCM2835_SPI->fifo), &tft_cmds[4*i+2], 8, 6); // 2e) send transfer length + 8 byte
            bcm2835_dma_fill_conblk(blk++, NULL, (void*) &(BCM2835_SPI->fifo), 4, 7); // 2f) read 4 bytes
        }
    }

    bcm2835_dma_fill_conblk(blk++, (void*) &(BCM2835_SPI->fifo), &tft_cmds[10], 4, 6); // send transfer length
    
    bcm2835_dma_fill_conblk(blk++, (void*) (0x20007000 + ((rx_dma_channel)<<8)), &tft_cmds[11], 4, 0); // start RX DMA channel
    
    
    bcm2835_dma_fill_conblk(blk++, (void*) &(BCM2835_SPI->fifo), swapbuffer, nbytes, 6); // send nbytes bytes

    bcm2835_dma_fill_conblk(&rx_conblk[0], NULL, (void*) &(BCM2835_SPI->fifo), nbytes, 7); // receive nbytes bytes

    rx_conblk[0].ti |= 1; // interrupt enable

    // chain tx blocks (all but the last)
    for (int i = 0; i< 20; i++) bcm2835_dma_chain_conblk(&tx_conblk[i], &tx_conblk[i+1]);
    
    bcm2835_dma_reset(rx_dma_channel);
    bcm2835_dma_reset(tx_dma_channel);
    
    BCM2835_SPI->cs = BCM2835_SPI_CS_DMAEN | BCM2835_SPI_CS_ADCS;

    // Hack: this is bcm2835_dma_start (loading of conblk pointer) without actually starting it
    (* (volatile uint32_t *) (0x20007004 + ((rx_dma_channel)<<8))) = (uint32_t) &rx_conblk[0] | 0x40000000;
    

    bcm2835_dma_start(tx_dma_channel, &tx_conblk[0]);
    
}

static int initialized = 0;
static SPIConfig cfg;

void sendcommand(unsigned char command, int data_size, const char *data) {
    GPCLR0 = 1<<25;
    spiSelect(&SPI0);
    spiSend(&SPI0, 1, &command);
    GPSET0 = 1<<25;
    if (data_size) spiSend(&SPI0, data_size, data);
    spiUnselect(&SPI0);
}

#define PITFT_INIT_DOC \
"init()\n" \
"\n" \
"Initialize the PiTFT display and connect it to the LittlevGL library\n"
static PyObject *
pitft_init(PyObject *self, PyObject *args) {
    bcm2835_gpio_fnsel(25, GPFN_OUT); // Data/command
    bcm2835_gpio_fnsel(18, GPFN_OUT); // Backlight
    GPSET0 = 1<<18; // Backlight on

    cfg.chip_select = 0;
    cfg.clock_polarity = 0;
    cfg.clock_phase = 0;
    cfg.clock_frequency = 20000000;
    cfg.end_cb = pitft_transfer_done;
    spiStart(&SPI0, &cfg);
    
    sendcommand(0x01, 0, NULL);
    chThdSleepMilliseconds(5);
    sendcommand(0x28, 0, NULL);
    sendcommand(0xcf, 3, "\x00\x83\x30");
    sendcommand(0xed, 4, "\x64\x03\x12\x81");
    sendcommand(0xe8, 3, "\x85\x01\x79");
    sendcommand(0xcb, 5, "\x39\x2c\x00\x34\x02");
    sendcommand(0xf7, 1, "\x20");
    sendcommand(0xea, 2, "\x00\x00");
    sendcommand(0xc0, 1, "\x26");
    sendcommand(0xc1, 1, "\x11");
    sendcommand(0xc5, 2, "\x35\x3e");
    sendcommand(0xc7, 1, "\xbe");
    sendcommand(0x3a, 1, "\x55");
    sendcommand(0xb1, 2, "\x00\x1b");
    sendcommand(0xf2, 1, "\x08");
    sendcommand(0xb7, 1, "\x07");
    sendcommand(0xb6, 4, "\x0a\x82\x27\x00");
    sendcommand(0x11, 0, NULL);
    chThdSleepMilliseconds(100);
    sendcommand(0x29, 0, NULL);
    chThdSleepMilliseconds(20);
    sendcommand(0x36, 1, "\xe0");
    sendcommand(0x2a, 4, "\x00\x00\x01\x3f");
    sendcommand(0x2b, 4, "\x00\x00\x00\xef");    
    
    Py_RETURN_NONE;
}



#define PITFT_COMMAND_DOC \
"command(command:integer, data: bytes)\n"

static PyObject *
pitft_command(PyObject *self, PyObject *args) {
    unsigned char command;
    const char *data;
    int data_size;
    
    if (!PyArg_ParseTuple(args, "by#", &command, &data, &data_size)) {
        return NULL;
    }
    
    sendcommand(command, data_size, data);

    
    Py_RETURN_NONE;
}
static lv_disp_drv_t driver = {0};
static int downsample = 0;
static int connected = 0;
static int pending = 0;

static PyObject *
pitft_connect(PyObject *self, PyObject *args) {
    driver.disp_flush = pitft_update;
    lv_disp_set_active(lv_disp_drv_register(&driver));
    connected = 1;
    Py_RETURN_NONE;
}

int poll(void * dummy) {
    lv_tick_inc(20);
    lv_task_handler();
    pending = 0;
    return 0;
}



void app_systick(void) {
    // TODO: neat way to register systick handler
    if (!connected) return;
    if (downsample++ >=20) {
        downsample = 0;
        if (!pending) {
            pending = 1;
            Py_AddPendingCall(poll, NULL);
        }
    }
}


static PyObject *
pitft_poll(PyObject *self, PyObject *args) {
    lv_tick_inc(50);
    lv_task_handler();

    Py_RETURN_NONE;
}




static PyMethodDef PitftMethods[] = {
    {"init",  pitft_init, METH_NOARGS, PITFT_INIT_DOC},
    {"command",  pitft_command, METH_VARARGS, PITFT_COMMAND_DOC},
    {"connect", pitft_connect, METH_NOARGS, NULL},
    {"poll", pitft_poll, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


static struct PyModuleDef pitftmodule = {
    PyModuleDef_HEAD_INIT,
    "pitft",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    PitftMethods
};

PyMODINIT_FUNC
PyInit_pitft(void) {

    PyObject *module = NULL;
    
    // Ensure lvgl module is loaded (and thus lvgl c-library is initialized)
    module = PyImport_ImportModule("lvgl");
    if (!module) goto error;
    Py_DECREF(module);
    
    module = PyModule_Create(&pitftmodule);
    if (!module) goto error;

    return module;
    
error:

    Py_XDECREF(module);
    return NULL;
}


