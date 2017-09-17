#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <poll.h>
#include <sys/time.h>

#include <linux/input.h>

const int the_keyboard_steps[] = {0, 8, 16, 32, 64, 128, 192, 255};
size_t the_keyboard_numsteps =
    sizeof(the_keyboard_steps) / sizeof(the_keyboard_steps[0]);

const int the_screen_steps[] = {0,   16,  32,  48,  64,   96,   128,  192,
                                256, 384, 512, 768, 1024, 1536, 2048, 2777};
size_t the_screen_numsteps =
    sizeof(the_screen_steps) / sizeof(the_screen_steps[0]);

const char the_input_path[] =
    "/dev/input/by-id/"
    "usb-Apple_Inc._Apple_Internal_Keyboard___Trackpad_DQ64222B5JXF94QAJCA-"
    "if01-event-kbd";

const char the_keyboard_path[] =
    "/sys/devices/platform/applesmc.768/leds/smc::kbd_backlight/brightness";

const char the_screen_path[] =
    "/sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-eDP-1/"
    "intel_backlight/brightness";

static int query(const char *path) {
  FILE *f;
  int v;

  if (NULL == (f = fopen(path, "r"))) {
    err(EXIT_FAILURE, "fopen %s", path);
  }

  if (1 != fscanf(f, "%d\n", &v)) {
    err(EXIT_FAILURE, "fscanf");
  }

  if (EOF == fclose(f)) {
    err(EXIT_FAILURE, "fclose");
  };

  return v;
}

static void update(FILE *f, int v) {
  if (0 > fprintf(f, "%d\n", v)) {
    err(EXIT_FAILURE, "fprintf");
  }

  if (EOF == fflush(f)) {
    err(EXIT_FAILURE, "fflush");
  };
}

static int step(int x, const int *steps, size_t numsteps, int direction) {
  if (1 == direction) {
    for (size_t i = 0; i < numsteps; ++i) {
      if (x < steps[i]) {
        return steps[i];
      };
    }
    return x;
  }

  for (size_t i = 0; i < numsteps; ++i) {
    size_t j;

    j = numsteps - 1 - i;

    if (steps[j] < x) {
      return steps[j];
    };
  }
  return x;
}

int main() {
  FILE *input_file;
  FILE *keyboard_file;
  FILE *screen_file;
  struct input_event ie;
  int keyboard_brightness;
  int screen_brightness;

  if (NULL == (input_file = fopen(the_input_path, "r"))) {
    err(EXIT_FAILURE, "fopen %s", the_input_path);
  }

  keyboard_brightness = query(the_keyboard_path);
  screen_brightness = query(the_screen_path);

  if (NULL == (keyboard_file = fopen(the_keyboard_path, "w"))) {
    err(EXIT_FAILURE, "fopen %s", the_keyboard_path);
  }

  if (NULL == (screen_file = fopen(the_screen_path, "w"))) {
    err(EXIT_FAILURE, "fopen %s", the_screen_path);
  }

  struct timeval last_update_tv = {0};

  for (;;) {
    // Wait for keypress or timeout
    int numevents;
    {
      int input_fd = fileno(input_file);
      struct pollfd pfd = {.fd = input_fd, .events = POLLIN};

      if (-1 == (numevents = poll(&pfd, 1, 1000))) {
        err(EXIT_FAILURE, "poll");
      }
    }

    struct timeval now_tv;

    if (-1 == gettimeofday(&now_tv, NULL)) {
      err(EXIT_FAILURE, "gettimeofday");
    }

    if (0 == numevents) {
      goto no_command;
    }

    if (1 != fread(&ie, sizeof(ie), 1, input_file)) {
      err(EXIT_FAILURE, "fread");
    }

    if (ie.value < 1 || 2 < ie.value) {
      goto no_command;
    }

    switch (ie.code) {
      case KEY_BRIGHTNESSDOWN:
        screen_brightness =
            step(screen_brightness, the_screen_steps, the_screen_numsteps, -1);
        break;
      case KEY_BRIGHTNESSUP:
        screen_brightness =
            step(screen_brightness, the_screen_steps, the_screen_numsteps, 1);
        break;
      case KEY_KBDILLUMDOWN:
        keyboard_brightness = step(keyboard_brightness, the_keyboard_steps,
                                   the_keyboard_numsteps, -1);
        break;
      case KEY_KBDILLUMUP:
        keyboard_brightness = step(keyboard_brightness, the_keyboard_steps,
                                   the_keyboard_numsteps, 1);
        break;
      default:
        goto no_command;
    }

    update(screen_file, screen_brightness);
    update(keyboard_file, keyboard_brightness);
    last_update_tv = now_tv;
    fprintf(stderr, "screen %4d; keyboard %3d\n", screen_brightness,
            keyboard_brightness);
    continue;

  no_command:
    if (last_update_tv.tv_sec < now_tv.tv_sec) {
      update(screen_file, screen_brightness);
      update(keyboard_file, keyboard_brightness);
      last_update_tv = now_tv;
    }
  }

  return EXIT_SUCCESS;
}
