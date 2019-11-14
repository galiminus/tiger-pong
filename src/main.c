#include <tonc.h>

const TILE racket_tiles[2] = {
  {{0x00011000, 0x00122100, 0x00133100, 0x00122100,
    0x00133100, 0x00122100, 0x00133100, 0x00122100}},
  {{0x00133100, 0x00122100, 0x00133100, 0x00122100,
    0x00133100, 0x00122100, 0x00133100, 0x00011000}},
};

const TILE ball_tile = {
  { 0x00111100,
    0x01222210,
    0x12222221,
    0x12222221,
    0x12222221,
    0x12222221,
    0x01222210,
    0x00111100
  }
};

#include "p1_played.h"
#include "p2_played.h"
#include "p1_won.h"
#include "p2_won.h"
#include "pause.h"

OBJ_ATTR obj_buffer[128];

enum state {
  PLAYING = 1,
  P1_WON,
  P2_WON,
  PAUSE
};

#define VEL_BITS 4
#define COLLISION_ACCELL 2
#define SPEED 2

struct racket {
  OBJ_ATTR * object;
  int x;
  int y;
};

struct ball {
  OBJ_ATTR * object;
  int x_vel;
  int y_vel;
  int x;
  int y;
  int scr_x;
  int scr_y;
};

void main() {
  struct racket rackets[2];
  struct ball ball;

  int ri;
  int rand_accel_y = -COLLISION_ACCELL;

  enum state state = PAUSE;
  enum state oldstate = PLAYING;
  int pause_time;

  ball.x = (240 / 2 - 4) << VEL_BITS;
  ball.y = (160 / 2 - 4) << VEL_BITS;
  ball.x_vel = 1 << VEL_BITS;
  ball.y_vel = 1 << VEL_BITS;

  REG_DISPCNT = DCNT_MODE3 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

  tile_mem[4][512] = racket_tiles[0];
  tile_mem[4][513] = racket_tiles[1];
  tile_mem[4][514] = ball_tile;

  oam_init(obj_buffer, 128);

  pal_obj_bank[4][1] = RGB15(0, 0, 0);
  pal_obj_bank[4][2] = RGB15(1, 22, 31);
  pal_obj_bank[4][3] = RGB15(24, 24, 28);

  pal_obj_bank[5][1] = RGB15(0, 0, 0);
  pal_obj_bank[5][2] = RGB15(18, 18, 18);
  pal_obj_bank[5][3] = RGB15(9, 9, 9);

  pal_obj_bank[6][1] = RGB15(0, 0, 0);
  pal_obj_bank[6][2] = RGB15(24, 4, 4);

  rackets[0].object = &obj_buffer[0];
  rackets[1].object = &obj_buffer[1];
  ball.object = &obj_buffer[2];

  obj_set_attr(rackets[0].object, ATTR0_TALL, ATTR1_SIZE_8, ATTR2_PALBANK(4) | 512);
  obj_set_attr(rackets[1].object, ATTR0_TALL, ATTR1_SIZE_8, ATTR2_PALBANK(5) | 512);
  obj_set_attr(ball.object, ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(6) | 514);

  rackets[0].x = 10;
  rackets[0].y = 10;

  rackets[1].x = 223;
  rackets[1].y = 10;

  // turn sound on
  REG_SNDSTAT= SSTAT_ENABLE;
  // snd1 on left/right ; both full volume
  REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1, 7);
  // DMG ratio to 100%
  REG_SNDDSCNT= SDS_DMG100;

    // no sweep
  REG_SND1SWEEP= SSW_OFF;
  // envelope: vol=12, decay, max step time (7) ; 50% duty
  REG_SND1CNT= SSQR_ENV_BUILD(12, 0, 2) | SSQR_DUTY1_2;
  REG_SND1FREQ= 0;

  while (1) {
    vid_vsync();
    key_poll();

    if (state != PLAYING && pause_time == 0 && ~REG_KEYINPUT & 0xff) {
      oldstate = state;
      state = PLAYING;
      if (ball.x_vel < 0) {
        tonccpy((u16 *)MEM_VRAM, p2_played, P2_PLAYED_SIZE);
      } else {
        tonccpy((u16 *)MEM_VRAM, p1_played, P1_PLAYED_SIZE);
      }
    }

    if (state == PAUSE && state != oldstate) {
      tonccpy((u16 *)MEM_VRAM, pause, PAUSE_SIZE);
    }

    if (state == P1_WON && state != oldstate) {
      tonccpy((u16 *)MEM_VRAM, p1_won, P1_WON_SIZE);
    }

    if (state == P2_WON && state != oldstate) {
      tonccpy((u16 *)MEM_VRAM, p2_won, P2_WON_SIZE);
    }

    if (state == PLAYING) {
      ball.x += ball.x_vel;
      ball.y += ball.y_vel;

      ball.scr_x = ball.x >> VEL_BITS;
      ball.scr_y = ball.y >> VEL_BITS;

      // p2 win
      if (ball.scr_x <= 0) {
        ball.x = (240 / 2 - 4) << VEL_BITS;
        ball.y = (160 / 2 - 4) << VEL_BITS;
        ball.x_vel = 1 << VEL_BITS;
        ball.y_vel = 1 << VEL_BITS;
        oldstate = state;
        state = P2_WON;
        pause_time = 60;
      }

      // p1 win
      if (ball.scr_x >= 232) {
        ball.x = (240 / 2 - 4) << VEL_BITS;
        ball.y = (160 / 2 - 4) << VEL_BITS;
        ball.x_vel = -(1 << VEL_BITS);
        ball.y_vel = 1 << VEL_BITS;
        oldstate = state;
        state = P1_WON;
        pause_time = 60;
      }

      if (ball.scr_y <= 0 || ball.scr_y >= 152) {
        ball.y_vel = -ball.y_vel;
      }

      // Racket detection
      if (ball.scr_x <= rackets[0].x + 6) {
        if (ball.scr_y >= rackets[0].y - 8 && ball.scr_y < rackets[0].y + 24) {
          ball.x_vel = -ball.x_vel + COLLISION_ACCELL;
          if (rand_accel_y < 0) {
            ball.y_vel = -ball.y_vel;
          }
          ball.y_vel += rand_accel_y;

          ball.x += ball.x_vel;
          ball.y += ball.y_vel;
          REG_SND1FREQ = SFREQ_RESET | SND_RATE(NOTE_D, 2);
          tonccpy((u16 *)MEM_VRAM, p1_played, P1_PLAYED_SIZE);
        }
      }
      if (ball.scr_x >= rackets[1].x - 6) {
        if (ball.scr_y >= rackets[1].y - 8 && ball.scr_y < rackets[1].y + 24) {
          ball.x_vel = -ball.x_vel - COLLISION_ACCELL;
          if (rand_accel_y < 0) {
            ball.y_vel = -ball.y_vel;
          }
          ball.y_vel += rand_accel_y;

          ball.x += ball.x_vel;
          ball.y += ball.y_vel;
          REG_SND1FREQ = SFREQ_RESET | SND_RATE(NOTE_D, 2);
          tonccpy((u16 *)MEM_VRAM, p2_played, P2_PLAYED_SIZE);
        }
      }
      // IA
      if (ball.x_vel > 0) {
        if (ball.scr_y + 4 < rackets[1].y + 8 - SPEED) {
          rackets[1].y -= SPEED;
        } else if (ball.scr_y + 4 > rackets[1].y + 8 + SPEED) {
          rackets[1].y += SPEED;
        }
      }

      rackets[0].y += key_tri_vert() * SPEED;

      // racket limits
      for (ri = 0; ri < 2; ri++) {
        if (rackets[ri].y < 0) {
          rackets[ri].y = 0;
        }
        if (rackets[ri].y > 144) {
          rackets[ri].y = 144;
        }
      }

      obj_set_pos(rackets[0].object, rackets[0].x, rackets[0].y);
      obj_set_pos(rackets[1].object, rackets[1].x, rackets[1].y);
      obj_set_pos(ball.object, ball.scr_x, ball.scr_y);

      oam_copy(oam_mem, obj_buffer, 3);

      rand_accel_y++;
      if (rand_accel_y > COLLISION_ACCELL << 1) {
        rand_accel_y = -(COLLISION_ACCELL << 1);
      }
    }
    if (pause_time > 0) {
      pause_time--;
    }
  }
}
