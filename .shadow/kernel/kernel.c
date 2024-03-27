#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>


#define SIDE 16

static int w, h;  // Screen size

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = { AM_KEYS(KEYNAME) };

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;

  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); // white
      }
    }
  }
}

void L0(){
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;

  int x = 0;
  int y = 0;
  for (; x * SIDE <= w/4; x ++) {
    for (; y * SIDE <= h/4; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); // white
      }
    }
  }
  for (; x * SIDE <= w/4; x ++) {
    for (; y * SIDE <= h/2; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xff0000); // red
      }
    }
  }
  for (; x * SIDE <= w/4; x ++) {
    for (; y * SIDE <= h/4*3; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x00ff00); // green
      }
    }
  }
  for (; x * SIDE <= w/4; x ++) {
    for (; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x0000ff); // blue
      }
    }
  }
  for (; x * SIDE <= w/2; x ++) {
    for (; y * SIDE <= h/4; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffff00); // yellow
      }
    }
  }
  for (; x * SIDE <= w/2; x ++) {
    for (; y * SIDE <= h/2; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xff00ff); // purple
      }
    }
  }
  for (; x * SIDE <= w/2; x ++) {
    for (; y * SIDE <= h/4*3; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x00ffff); // cyan
      }
    }
  }
  for (; x * SIDE <= w/2; x ++) {
    for (; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x000000); // black
      }
    }
  }
  for (; x * SIDE <= w/4*3; x ++) {
    for (; y * SIDE <= h/4; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x7f7f7f); // gray
      }
    }
  }
  for (; x * SIDE <= w/4*3; x ++) {
    for (; y * SIDE <= h/2; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x7f0000); // dark red
      }
    }
  }
  for (; x * SIDE <= w/4*3; x ++) {
    for (; y * SIDE <= h/4*3; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x007f00); // dark green
      }
    }
  }
  for (; x * SIDE <= w/4*3; x ++) {
    for (; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x00007f); // dark blue
      }
    }
  }
  for (; x * SIDE <= w; x ++) {
    for (; y * SIDE <= h/4; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x7f7f00); // dark yellow
      }
    }
  }
  for (; x * SIDE <= w; x ++) {
    for (; y * SIDE <= h/2; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x7f007f); // dark purple
      }
    }
  }
  for (; x * SIDE <= w; x ++) {
    for (; y * SIDE <= h/4*3; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x007f7f); // dark cyan
      }
    }
  }
  for (; x * SIDE <= w; x ++) {
    for (; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x7f7f7f); // gray
      }
    }
  }

}

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  // splash();
  L0();


  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}
