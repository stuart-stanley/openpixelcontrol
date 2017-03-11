#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ws2811.h"
#include "opc.h"

#define POST_TX_DELAY_USECS 1000
// I'm seeing flickering at speeds above ~6mhz. Although the 
// WS2801 datasheet lists a max rate of 25mhz, anecdotal evidence 
// from forum posts suggests that most people have had trouble 
// obtaining that in practice.
#define WS2811_DEFAULT_SPEED 8000000

static ws2811_t ws2811_handle;

static u32 spi_speed_hz = WS2811_DEFAULT_SPEED;
static u8 buffer[1 << 16];
static void exit_nicely(const ws2811_return_t state) {
  ws2811_fini(&ws2811_handle);
  exit(state);
}

void ws2811_put_pixels(u8 buffer[], u16 count, pixel* pixels) {
  int chan = -1;
  int left_in_chan = 0;
  int led_inx;
  ws2811_led_t *leds;
  uint32_t color;
  pixel *p;
  ws2811_return_t ecode;

  p = pixels;
  //printf("count=%d, pixels=%x\n", count, pixels);
  for (led_inx = 0; led_inx < count; led_inx++) {
    if ( left_in_chan == 0 ) {
      chan++;
      leds = ws2811_handle.channel[chan].leds;
      left_in_chan = ws2811_handle.channel[chan].count;
    }
    color = (p->r) >> 16;
    color |= ((p->g) >> 8);
    color |= (p->b);
    *leds = color;
    //printf("leds=%p, color=%x\n", leds, color);
    leds++;
    left_in_chan--;
  }
  ecode = ws2811_render(&ws2811_handle);
  if (ecode != WS2811_SUCCESS) {
    fprintf(stderr, "render failed %s\n", ws2811_get_return_t_str(ecode));
    exit_nicely(ecode);
  }
  ecode = ws2811_wait(&ws2811_handle);
  if (ecode != WS2811_SUCCESS) {
    fprintf(stderr, "wait failed %s\n", ws2811_get_return_t_str(ecode));
    exit_nicely(ecode);
  }
}


static void setup_chan(int chan, int count, int gpionum) {
  ws2811_channel_t *chan_p;

  chan_p = &ws2811_handle.channel[chan];

  chan_p->gpionum = gpionum;
  chan_p->invert = 0;
  chan_p->count = count;
  chan_p->brightness = 255;
  chan_p->strip_type = WS2811_STRIP_RGB;
}



void opc_serve_handler(u8 address, u16 count, pixel* pixels) {
  fprintf(stderr, "%d ", count);
  fflush(stderr);

  ws2811_put_pixels(buffer, count, pixels);
}

#define DIAG_P_COUNT 200
#define DIAGNOSTIC_TIMEOUT_MS 1000

static int main_loop(u16 port) {
  pixel diagnostic_pixels[DIAG_P_COUNT];
  time_t t;
  u16 inactivity_ms = 0;
  int i;

  opc_source s = opc_new_source(port);
  if (s < 0) {
    fprintf(stderr, "Could not create OPC source\n");
    return 1;
  }
  fprintf(stderr, "Ready...\n");
  for (i = 0; i < DIAG_P_COUNT; i++) {
    diagnostic_pixels[i].r = 0;
    diagnostic_pixels[i].g = 0;
    diagnostic_pixels[i].b = 0;
  }
  while(1) {
    if (opc_receive(s, opc_serve_handler, DIAGNOSTIC_TIMEOUT_MS)) {
      inactivity_ms = 0;
    } else {
      inactivity_ms += DIAGNOSTIC_TIMEOUT_MS;
      for (i = 0; i < DIAG_P_COUNT; i++ ) {
	t = time(NULL);
	diagnostic_pixels[i].r = (t % 3 == 0) ? 64 : 0;
	diagnostic_pixels[i].g = (t % 3 == 1) ? 64 : 0;
	diagnostic_pixels[i].b = (t % 3 == 2) ? 64 : 0;
      }
      ws2811_put_pixels(buffer, DIAG_P_COUNT, diagnostic_pixels);
    }
  }
}

int main(int argc, char** argv) {
  u16 port = OPC_DEFAULT_PORT;
  ws2811_return_t rcode;

  /*
    _LED1_COUNT      = 100     # Number of LED pixels.
    _LED1_PIN        = 19      # GPIO pin connected to the pixels (must support PWM! GPIO 13 and 18 on RPi 3).
    _LED1_FREQ_HZ    = 800000  # LED signal frequency in hertz (usually 800khz)
    _LED1_DMA        = 5       # DMA channel to use for generating signal (Between 1 and 14)
    _LED1_BRIGHTNESS = 255     # Set to 0 for darkest and 255 for brightest
    _LED1_INVERT     = False   # True to invert the signal (when using NPN transistor level shift)
    _LED1_CHANNEL    = 1       # 0 or 1
    _LED1_STRIP      = ws.WS2811_STRIP_GRB

    _LED2_COUNT      = 100      # Number of LED pixels.
    _LED2_PIN        = 18      # GPIO pin connected to the pixels (must support PWM! GPIO 13 or 18 on RPi 3).
    _LED2_FREQ_HZ    = 800000  # LED signal frequency in hertz (usually 800khz)
    _LED2_DMA        = 5      # DMA channel to use for generating signal (Between 1 and 14)
    _LED2_BRIGHTNESS = 255     # Set to 0 for darkest and 255 for brightest
    _LED2_INVERT     = False   # Tru
    channle = 1
    strip = ws.ws2811_strip_grb
  */
  memset(&ws2811_handle, 0, sizeof(ws2811_handle));
  ws2811_handle.dmanum = 5;
  ws2811_handle.freq = 800000;
    
  setup_chan(0, 100, 18);
  setup_chan(1, 100, 19);
  rcode = ws2811_init(&ws2811_handle);
  if (rcode != WS2811_SUCCESS) {
    fprintf(stderr, "init failed %s\n", ws2811_get_return_t_str(rcode));
    exit_nicely(rcode);
  }
  
  return main_loop(port);
}
