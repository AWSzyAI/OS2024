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
    if(event.keycode == AM_KEY_ESCAPE){
      halt(0);
    }
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


//szy.jpg 800px*600px
//szy.jpg: JPEG image data, JFIF standard 1.01, resolution (DPI), 
//density 120x120, segment length 16, baseline, precision 8, 800x600, components 3

void draw_file(char *path){
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
  //w h support 320×200、640×480、800×600px

  // read the file,judge jpg, png...
  // read the file, get the width and height of the picture
  // read the file, get the pixel data of the picture
  // draw the picture on the screen
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = 0, .y = 0, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };

  //解析path对应的jpg文件，获取像素数据
  int color = 0xff0000;
  for(int j = 0; j < h; j+=h/10){
    for(int i = 0; i < w; i++){
      pixels[i] = color;
    }
    color = color + 0x0000ff;
  }
  
  ioe_write(AM_GPU_FBDRAW, &event);

}

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  // splash();
  draw_file("./szy.png");


  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}
