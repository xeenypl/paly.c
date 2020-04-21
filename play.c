#include <ao/ao.h>
#include <mpg123.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include <time.h>

#define BITS 8

bool play_loop = false;
bool play_random = false;

struct termios orig_termios;

static void reset_terminal_mode() {
  tcsetattr(0, TCSANOW, &orig_termios);
}

static void set_terminal_mode() {
  struct termios new_termios;
  tcgetattr(0, &orig_termios);
  memcpy(&new_termios, &orig_termios, sizeof(new_termios));

  atexit(reset_terminal_mode);
  cfmakeraw(&new_termios);
  new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  new_termios.c_oflag &= ~(OPOST);
  new_termios.c_cflag |= (CS8);
  new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  tcsetattr(0, TCSANOW | TCSAFLUSH , &new_termios);
}

static int kbhit() {
  struct timeval tv = { 0L, 0L };
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  return select(1, &fds, NULL, NULL, &tv);
}

static int getch() {
  int r;
  unsigned char c;
  if ((r = read(0, &c, sizeof(c))) < 0)
    return r;
  else
    return c;
}

static void drow_screan(int len, char **file, int now) {
  printf("\x1b[2J\x1b[H");
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    fprintf(stderr, "can not get twemianl size.\n\r");
    exit(2);
  }
  int offset = 0;

  if (ws.ws_row < len) {
    if (len - (now - ws.ws_row / 2) < ws.ws_row) {
      offset = len - ws.ws_row + 1;  
    } else {
      if (now > ws.ws_row / 2)
        offset = now - ws.ws_row / 2; 
    }
  }

  for (int i = 0; i + offset < len && i < ws.ws_row - 1; i++) 
    printf("%s%s\033[0m\n\r", 
        i + offset == now ? "\033[31m" : "",
        basename(file[i + offset]));

  printf("\033[%d;1H (%d/%d) %s%s\r",
      ws.ws_row, 
      now + 1, len,
      play_loop ? "Loop " : "",
      play_random ? "Random " : "");
  fflush(stdout);
}

static void using() {
}

int main(int argc, char **argv) {
  int driver, ch;
  ao_device *dev;
  ao_sample_format format;
  while ((ch = getopt(argc, argv, "rl")) != -1)
    switch (ch) {
      case 'r': break;
      case 'l': play_loop = true; break;
      default:
        using();
    }
  argc -= optind; argv += optind;

  if(argc < 1) using();
  
  srand((unsigned) time(NULL));
  set_terminal_mode();
  ao_initialize();
  driver = ao_default_driver_id();

  do {
    for (int i = 0; i < argc; i++) {
      int err, channels, encoding;
      mpg123_init();
      mpg123_handle *mh = mpg123_new(NULL, &err);
      size_t buffer_size = mpg123_outblock(mh);
      unsigned char *buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
      size_t done;
      long rate;

      if (play_random) 
        i = rand() % argc;
      mpg123_open(mh, argv[i]);

      drow_screan(argc, argv, i);

      mpg123_getformat(mh, &rate, &channels, &encoding);
      format.bits = mpg123_encsize(encoding) * BITS;
      format.rate = rate;
      format.channels = channels;
      format.byte_format = AO_FMT_NATIVE;
      format.matrix = 0;
      dev = ao_open_live(driver, &format, NULL);

      bool playng = true;
      bool pause = false;

      while (playng) {
        if (!pause) { 
          if(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
            ao_play(dev, buffer, done);
          else
            break;
        }
        if (kbhit()) {
          char ch = getch();
          switch (ch) {
            case 3:   // C-c
            case 113: // q
              exit(0);
              break;
            case 110: // n
              if (i > argc - 1) i = argc - 1;
              playng = 0;
              break;
            case 112: // p
              i -= 2;
              if (i < -1) i = -1;
              playng = 0;
              break;
            case 108: // l
              play_loop = !play_loop;
              drow_screan(argc, argv, i);
              break;
            case 114: // r
              play_random = !play_random;
              drow_screan(argc, argv, i);
              break;
            case 32:  // space
              pause = !pause;
              break;
          }
        }
      }

      free(buffer);
      mpg123_close(mh);
      mpg123_delete(mh);
      mpg123_exit();
    }
  } while (play_loop);
  ao_close(dev);
  ao_shutdown();

  return 0;
}
