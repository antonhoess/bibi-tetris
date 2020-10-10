#include <cairo.h>
#include <canberra.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#define AREA_WIDTH      10
#define AREA_HEIGHT     20
#define AREA_HEIGHT_EXT (AREA_HEIGHT + 2)
#define TYPES_CNT        7


typedef struct block_s {
  int set;
  int fixed;
  int color;
} block_t;

typedef struct point_s {
  int x;
  int y;
} point_t;

typedef struct figure_s {
  int type;
  int rotatable; // 0 = No, 1 = Yes, full, 2 = Yes, half, right, 3 = Yes, half, left
  int rotation;
  point_t blocks[4];
  point_t pos; // Y from upper border
} figure_t;

typedef struct color_s {
  float r;
  float g;
  float b;
} color_t;

typedef struct colorset_s {
  color_t c0;
  color_t c1;
  color_t c2;
} colorset_t;

typedef enum {
  STATE_INIT,
  STATE_SELECT_LEVEL,
  STATE_GAME_PLAY,
  STATE_GAME_FULL_ROW,
  STATE_GAME_PAUSE,
  STATE_GAME_CLOSING,
  STATE_GAME_FINISHED,
} modes_t;


static void
do_drawing(cairo_t *);

static void
set_level_speed();

static int
check_figure(figure_t *f);

static gboolean
close_game(gpointer user_data);

static void
refresh();


figure_t   figures[TYPES_CNT];
block_t    area[AREA_WIDTH][AREA_HEIGHT_EXT];
colorset_t colorsets[10];
int type_color_assoziation[TYPES_CNT] = {0, 0, 0, 2, 1, 1, 2};

ca_context *ctx_sound = NULL;
/*XXX
ca_context *sound_translate = NULL;
ca_context *sound_rotate = NULL;
ca_context *sound_freeze = NULL;
*/

GtkWidget *window = NULL;
GtkWidget *darea = NULL;
point_t da_pos[4];
point_t ws; // Window-size

int BLOCK_DIM = 18;
int rows = 0;
int level = 0;
int level_preset = 0;
int points = 0;
figure_t f;
int ftp = -1; // Figure type (preview)
int stat_cnt[TYPES_CNT];
int preview_visible = 0;
modes_t mode = STATE_INIT;
int roll_cnt = 0;
int level_speed_id = 0;
int rows_full[4] = {-1, -1, -1, -1};
int row_del_effect_type = 0;
int row_del_effect_cnt = 0;

int key_down = 0;
int key_down_cnt = 0;
int key_down_line = 0;
int key_left = 0;
int key_left_cnt = 0;
int key_right = 0;
int key_right_cnt = 0;
int key_a = 0;
int key_a_cnt = 0;
int key_b = 0;
int key_b_cnt = 0;


static void
create_colorsets()
{
  colorset_t *cs = NULL;

  // Set 0
  cs = &colorsets[0];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.24, 0.74, 0.99}; // #3CBCFC
  cs->c2 = (color_t){0.13, 0.22, 0.93}; // #2038EC

  // Set 1
  cs = &colorsets[1];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.50, 0.82, 0.06}; // #80D010
  cs->c2 = (color_t){0, 0.6, 0};        // #00A800

  // Set 2
  cs = &colorsets[2];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.96, 0.46, 0.99}; // #F478FC
  cs->c2 = (color_t){0.74, 0, 0.74};    // #BC00BC

  // Set 3
  cs = &colorsets[3];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.30, 0.86, 0.28}; // #4CDC48
  cs->c2 = (color_t){0.13, 0.20, 0.93}; // #2038EC

  // Set 4
  cs = &colorsets[4];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.35, 0.97, 0.60}; // #58F898
  cs->c2 = (color_t){0.89, 0, 0.35};    // #E40058

  // Set 5
  cs = &colorsets[5];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.36, 0.58, 0.99}; // #5C94FC
  cs->c2 = (color_t){0.35, 0.97, 0.60}; // #58F898

  // Set 6
  cs = &colorsets[6];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.45, 0.45, 0.45}; // #747474
  cs->c2 = (color_t){0.85, 0.16, 0};    // #D82800

  // Set 7
  cs = &colorsets[7];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.6, 0, 0.06};     // #A80010
  cs->c2 = (color_t){0.5, 0, 0.94};     // #8000F0

  // Set 8
  cs = &colorsets[8];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.85, 0.16, 0};    // #D82800
  cs->c2 = (color_t){0.13, 0.20, 0.93}; // #2038EC

  // Set 9
  cs = &colorsets[9];
  cs->c0 = (color_t){1, 1, 1};
  cs->c1 = (color_t){0.99, 0.60, 0.22}; // #FC9838
  cs->c2 = (color_t){0.85, 0.16, 0};    // #D82800
}


static void
create_figures()
{
  figure_t *f = NULL;

  // Figure 1
  // XXOX
  f = &figures[0];

  f->type = 0;
  f->rotatable = 2;
  f->rotation = 0;

  f->blocks[0].x = -2;
  f->blocks[0].y = 0;
  f->blocks[1].x = -1;
  f->blocks[1].y = 0;
  f->blocks[2].x = 0;
  f->blocks[2].y = 0;
  f->blocks[3].x = 1;
  f->blocks[3].y = 0;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;

  // Figure 2
  // XO
  // XX
  f = &figures[1];

  f->type = 1;
  f->rotatable = 0;
  f->rotation = 0;

  f->blocks[0].x = -1;
  f->blocks[0].y = 0;
  f->blocks[1].x = 0;
  f->blocks[1].y = 0;
  f->blocks[2].x = -1;
  f->blocks[2].y = -1;
  f->blocks[3].x = 0;
  f->blocks[3].y =-1;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;

  // Figure 3
  // XOX
  //  X
  f = &figures[2];

  f->type = 2;
  f->rotatable = 1;
  f->rotation = 0;

  f->blocks[0].x = -1;
  f->blocks[0].y = 0;
  f->blocks[1].x = 0;
  f->blocks[1].y = 0;
  f->blocks[2].x = 1;
  f->blocks[2].y = 0;
  f->blocks[3].x = 0;
  f->blocks[3].y = -1;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;

  // Figure 4
  // XOX
  //   X
  f = &figures[3];

  f->type = 3;
  f->rotatable = 1;
  f->rotation = 0;

  f->blocks[0].x = -1;
  f->blocks[0].y = 0;
  f->blocks[1].x = 0;
  f->blocks[1].y = 0;
  f->blocks[2].x = 1;
  f->blocks[2].y = 0;
  f->blocks[3].x = 1;
  f->blocks[3].y = -1;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;

  // Figure 5
  // XOX
  // X
  f = &figures[4];

  f->type = 4;
  f->rotatable = 1;
  f->rotation = 0;

  f->blocks[0].x = -1;
  f->blocks[0].y = 0;
  f->blocks[1].x = 0;
  f->blocks[1].y = 0;
  f->blocks[2].x = 1;
  f->blocks[2].y = 0;
  f->blocks[3].x = -1;
  f->blocks[3].y = -1;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;

  // Figure 6
  // XO
  //  XX
  f = &figures[5];

  f->type = 5;
  f->rotatable = 2;
  f->rotation = 0;

  f->blocks[0].x = -1;
  f->blocks[0].y = 0;
  f->blocks[1].x = 0;
  f->blocks[1].y = 0;
  f->blocks[2].x = 0;
  f->blocks[2].y = -1;
  f->blocks[3].x = 1;
  f->blocks[3].y = -1;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;

  // Figure 7
  //  OX
  // XX
  f = &figures[6];

  f->type = 6;
  f->rotatable = 3;
  f->rotation = 0;

  f->blocks[0].x = 0;
  f->blocks[0].y = 0;
  f->blocks[1].x = 1;
  f->blocks[1].y = 0;
  f->blocks[2].x = -1;
  f->blocks[2].y = -1;
  f->blocks[3].x = 0;
  f->blocks[3].y = -1;

  f->pos.x = AREA_WIDTH / 2;
  f->pos.y = 0;
}


static void
clear_area()
{
  int x = 0, y = 0;
  block_t *b = NULL;

  for(y = 0; y < AREA_HEIGHT_EXT; y++) {
    for(x = 0; x < AREA_WIDTH; x++) {
      b = &area[x][y];
      b->set = 0;
      b->fixed = 0;
    }
  }
}


static void
figure_new()
{
  int i = 0;
  block_t *b = NULL;

  // Init the first preview
  if(ftp == -1)
    ftp = rand() % 7;

  // Move preview to area
  f.type = ftp;
  memcpy(&f, &figures[f.type], sizeof(figure_t));
  //f.rotatable
  f.pos.y = AREA_HEIGHT - 1;

  for(i = 0; i < 4; i++) {
    b = &area[f.blocks[i].x + f.pos.x][f.blocks[i].y + f.pos.y];
    b->set = 1;
    //b->fixed = 0;
    b->color = type_color_assoziation[f.type];
  }

  stat_cnt[f.type]++;

  // Set a new preview
  ftp = rand() % 7;
}


static void
remove_del_rows()
{
  int i = 0;
  int x = 0, y = 0;
  int new_rows = 0;
  int new_level = 0;

  // Delete full rows
  for(i = 3; i >= 0; i--) {
    if(rows_full[i] == -1)
      continue;

    new_rows++;

    for(y = rows_full[i] + 1; y < AREA_HEIGHT_EXT; y++) {
      for(x = 0; x < AREA_WIDTH; x++) {
        memcpy(&area[x][y - 1], &area[x][y], sizeof(block_t));
        if(y == AREA_HEIGHT_EXT - 1) {
          area[x][y].set = 0;
          area[x][y].fixed = 0;
        }
      }
    }
  }

  if (new_rows > 0) {
    for(i = 0; i < new_rows; i++) {
      if(new_rows == 1)
        points += 40 * (level + 1);
      else if(new_rows == 2)
        points += 100 * (level + 1);
      else if(new_rows == 3)
        points += 300 * (level + 1);
      else //if(new_rows == 4)
        points += 1200 * (level + 1);

      rows++;
      new_level = rows / 10;
      if(new_level > level) {
        level = new_level;
        ca_context_play(ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_LevelUp.ogg", NULL);
      }
    }
  }
}


static gboolean
row_del_effect(gpointer user_data)
{
  static int i = 0;

  if(++i >= 5) {
    i = 0;
    row_del_effect_type = 0;
    row_del_effect_cnt = 0;
    remove_del_rows();
    figure_new();
    key_down_line = f.pos.y;
    refresh();
    if(check_figure(&f)) {
      mode = STATE_GAME_CLOSING;
      g_source_remove(level_speed_id);
      level_speed_id = 0;
      g_timeout_add(100, close_game, NULL);
    }
    mode = STATE_GAME_PLAY;
    set_level_speed(0);
    return FALSE;
  } else {
    row_del_effect_cnt++;
    refresh();
    return TRUE;
  }
}


static int
check_row_full()
{
  int i = 0;
  int row = 0;
  int x = 0, y = 0;
  int old_level = level;
  int new_rows = 0;
  int blocks_per_row = 0;

  for(i = 0; i < 4; i++)
    rows_full[i] = -1;

  row = 0;
  for(y = 0; y < AREA_HEIGHT_EXT; y++) {
    blocks_per_row = 0;

    for(x = 0; x < AREA_WIDTH; x++) {
      if(area[x][y].fixed)
        blocks_per_row++;
    }

    if(blocks_per_row == AREA_WIDTH) {
      rows_full[row++] = y;

      if(++new_rows == 4)
        break;
    }
  }

  if(new_rows > 0) {
    if(new_rows == 1)
      ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_SpecialLineClearSingle.ogg", NULL);
    else if(new_rows == 2)
      ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_SpecialLineClearDouble.ogg", NULL);
    else if(new_rows == 3)
      ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_SpecialLineClearTriple.ogg", NULL);
    else //if(new_rows == 4)
      ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_SpecialTetris.ogg", NULL);

    mode = STATE_GAME_FULL_ROW;
    set_level_speed(1);

    if(new_rows < 4)
      row_del_effect_type = 1;
    else
      row_del_effect_type = 2;

    g_timeout_add(100, row_del_effect, NULL);
    return 1;
  } else {
    return 0;
  }
}


static int
figure_freeze(figure_t *f)
{
  int i = 0;
  point_t *b = NULL;
  point_t *p = NULL;

  p = &f->pos;
  for(i = 0; i < 4; i++) {
    b = &f->blocks[i];
    area[b->x + p->x][b->y + p->y].fixed = 1;
  }

  return check_row_full();
}


static gboolean
on_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr = NULL;
  cr = gdk_cairo_create(gtk_widget_get_window(widget));
//  cairo_translate(cr, -0.5, -0.5);
  do_drawing(cr);
//  cairo_translate(cr, -0.5, -0.5);
  cairo_destroy(cr);

  return FALSE;
}


static void
draw_block(cairo_t *cr, block_t *b, int x, int y)
{
  // Choose the right colorset
  colorset_t *cs = NULL;
  cs = &colorsets[level % 10];
  color_t *c = NULL;

  // Block
  switch(b->color) {
  case 0:
  default:
    c = &cs->c0;
    break;

  case 1:
    c = &cs->c1;
    break;

  case 2:
    c = &cs->c2;
    break;
  }

  cairo_set_source_rgb(cr, c->r, c->g, c->b);
  cairo_rectangle(cr, x * BLOCK_DIM + 1, -(y * BLOCK_DIM + 1), BLOCK_DIM - 1, -(BLOCK_DIM - 1));
  cairo_fill(cr);

  if(b->color == 0) {
   // Inner frame
    cairo_translate(cr, 0.5, -0.5);
    cairo_set_source_rgb(cr, cs->c2.r, cs->c2.g, cs->c2.b);
    cairo_rectangle(cr, x * BLOCK_DIM + 1, -(y * BLOCK_DIM + 1), BLOCK_DIM - 2, -(BLOCK_DIM - 2));
    cairo_stroke(cr);
    cairo_translate(cr, -0.5, 0.5);
  }

  // Gloss
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr, x * BLOCK_DIM + 1, -((y + 1) * BLOCK_DIM - 1), 1, -1);
  cairo_rectangle(cr, x * BLOCK_DIM + 2, -((y + 1) * BLOCK_DIM - 2), 2, -1);
  cairo_rectangle(cr, x * BLOCK_DIM + 2, -((y + 1) * BLOCK_DIM - 3), 1, -1);
  cairo_fill(cr);

  // Outer frame
  cairo_translate(cr, 0.5, -0.5);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_rectangle(cr, x * BLOCK_DIM, -(y * BLOCK_DIM), BLOCK_DIM, -BLOCK_DIM);
  cairo_stroke(cr);
  cairo_translate(cr, -0.5, 0.5);
}


static void
init_for_new_game()
{
  int i = 0;

  level = level_preset;
  rows = 0;
  points = 0;

  ftp = -1;

  for(i = 0; i < TYPES_CNT; i++)
    stat_cnt[i] = 0;

  clear_area();
}


static void
do_drawing(cairo_t *cr)
{
  int i = 0, k = 0;
  int x = 0, y = 0;
  block_t *b = NULL;
  figure_t *f = &figures[ftp];
  block_t bp;
  point_t *p = NULL;
  int row_for_del = 0;
  static char stat_cnt_str[10];
  static char info_str[20];

  cairo_set_line_width(cr, 1.0);

  // Window background
  cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
  cairo_rectangle(cr, 0, 0, ws.x, ws.y);
  cairo_fill(cr);

  // ## AREA Begin
  cairo_save(cr);
  cairo_translate(cr, da_pos[0].x, da_pos[0].y);

  // Area Background
  cairo_pattern_t *r1;
  cairo_save(cr);

  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_set_line_width(cr, 12);

  cairo_save(cr);
  cairo_translate(cr, 60, -160);
  r1 = cairo_pattern_create_radial(BLOCK_DIM * AREA_WIDTH * 3 / 4, -BLOCK_DIM * AREA_HEIGHT * 3 / 4, 10, 30, -BLOCK_DIM * AREA_HEIGHT * 3 / 4, BLOCK_DIM * 15);
  cairo_pattern_add_color_stop_rgb(r1, 0, 0.2, 0.2, 0.2);
  cairo_pattern_add_color_stop_rgb(r1, 1, 0.0, 0.0, 0.0);
  cairo_restore(cr);

  cairo_set_source(cr, r1);
  cairo_rectangle(cr, 0, 0, AREA_WIDTH * BLOCK_DIM + 1, -(AREA_HEIGHT * BLOCK_DIM + 1));
  cairo_fill(cr);

  cairo_pattern_destroy(r1);
  cairo_restore(cr);

  if(mode == STATE_GAME_PLAY || mode == STATE_GAME_CLOSING || mode == STATE_GAME_FINISHED || mode == STATE_GAME_FULL_ROW) {
    // Draw blocks
    for (y = 0; y < AREA_HEIGHT_EXT; y++ ) {
      // Check if current if for deletion
      row_for_del = 0;
      for(i = 0; i < 4; i++) {
        if(rows_full[i] == y) {
          row_for_del = 1;
          break;
        }
      }

      if(row_for_del && row_del_effect_type == 1) {
        int tmp = row_del_effect_cnt / 1;
        if(tmp > AREA_WIDTH / 2)
          tmp = AREA_WIDTH / 2;
        for (x = tmp; x < AREA_WIDTH - tmp; x++ ) {
          b = &area[x][y];
          if(b->set == 1) {
            draw_block(cr, b, x, y);
          }
        }
      } else {
        for (x = 0; x < AREA_WIDTH; x++ ) {
          b = &area[x][y];
          if(b->set == 1) {
            draw_block(cr, b, x, y);
          }
        }
      }
    }

    if(mode == STATE_GAME_FULL_ROW) {
      if(0 && row_del_effect_type == 1 && row_del_effect_cnt % 2 == 0) {
        for(i = 0; i < 4; i++) {
          if(rows_full[i] != -1) {
            for (x = 0 - (row_del_effect_cnt % 5); x < AREA_WIDTH - (row_del_effect_cnt % 5); x++ ) {
              cairo_set_source_rgb(cr, 1, 1, 1);
              cairo_rectangle(cr, x * BLOCK_DIM + 1, -(rows_full[i] * BLOCK_DIM + 1), BLOCK_DIM - 1, -(BLOCK_DIM - 1));
              cairo_fill(cr);
            }
          }
        }
      } else if(row_del_effect_type == 2 && row_del_effect_cnt % 2 == 0) {
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_rectangle(cr, 0, 0, AREA_WIDTH * BLOCK_DIM + 1, -(AREA_HEIGHT * BLOCK_DIM + 1));
        cairo_fill(cr);
      }
    }

    if(mode == STATE_GAME_CLOSING || mode == STATE_GAME_FINISHED) {
      // Game closed
      int width = AREA_WIDTH * BLOCK_DIM - 1;

      for(i = AREA_HEIGHT_EXT - 1; i >= AREA_HEIGHT_EXT - roll_cnt - 1; i--) {
        colorset_t *cs = NULL;
        cs = &colorsets[level % 10];
        color_t *c = NULL;

        c = &cs->c2;
        cairo_set_source_rgb(cr, c->r, c->g, c->b);
        cairo_rectangle(cr, 1, -(i * BLOCK_DIM + BLOCK_DIM - 2), width, -2);
        cairo_fill(cr);

        c = &cs->c0;
        cairo_set_source_rgb(cr, c->r, c->g, c->b);
        cairo_rectangle(cr, 1,  -(i * BLOCK_DIM + 3), width, -(BLOCK_DIM - 5));
        cairo_fill(cr);

        c = &cs->c1;
        cairo_set_source_rgb(cr, c->r, c->g, c->b);
        cairo_rectangle(cr, 1, -(i * BLOCK_DIM + 1), width, -2);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, 1, -(i * BLOCK_DIM), width, -1);
        cairo_fill(cr);
      }

      cairo_save(cr);
      cairo_translate(cr, 0.5, -0.5);

      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_rectangle(cr, 0, 0, width + 1, -(AREA_HEIGHT_EXT * BLOCK_DIM));
      cairo_stroke(cr);
      cairo_restore(cr);
    }
  } else if(mode == STATE_GAME_PAUSE) {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, BLOCK_DIM * 1.5);
    cairo_move_to(cr, 2 * BLOCK_DIM, - AREA_HEIGHT / 2 * BLOCK_DIM);
    cairo_show_text(cr, "PAUSE");
  } else if(mode == STATE_SELECT_LEVEL) {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, BLOCK_DIM * 0.6);
    cairo_move_to(cr, 1 * BLOCK_DIM, - AREA_HEIGHT / 2 * BLOCK_DIM + (-2 * BLOCK_DIM));
    cairo_show_text(cr, "Please Choose a");
    cairo_move_to(cr, 1 * BLOCK_DIM, - AREA_HEIGHT / 2 * BLOCK_DIM + (-1 * BLOCK_DIM));
    cairo_show_text(cr, "Starting Level (0..9)");
    cairo_move_to(cr, 2 * BLOCK_DIM, - AREA_HEIGHT / 2 * BLOCK_DIM + (0 * BLOCK_DIM));
    cairo_show_text(cr, "* Arrow Up: +1");
    cairo_move_to(cr, 2 * BLOCK_DIM, - AREA_HEIGHT / 2 * BLOCK_DIM + (1 * BLOCK_DIM));
    cairo_show_text(cr, "* Arrwo Down: -1");
  }

  cairo_restore(cr);
  // -> AREA End

  // ## PREVIEW
  cairo_save(cr);
  cairo_translate(cr, da_pos[1].x, da_pos[1].y);

  // Preview Background
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_rectangle(cr, 0, 0, 4 * BLOCK_DIM + 1, -(4 * BLOCK_DIM + 1));
  cairo_fill(cr);

  if((mode == STATE_GAME_PLAY || mode == STATE_GAME_FULL_ROW) && preview_visible && ftp >= 0) {
    f = &figures[ftp];
    bp.color = type_color_assoziation[ftp];

    if(ftp == 0)
      cairo_translate(cr, 0, BLOCK_DIM / 2);
    else if(ftp == 2 || ftp == 3 || ftp == 4 || ftp == 5 || ftp == 6)
      cairo_translate(cr, -BLOCK_DIM / 2, 0);

    for(i = 0; i < 4; i++) {
      p = &f->blocks[i];
      draw_block(cr, &bp, p->x + 2, p->y + 2);
    }

    if(ftp == 0)
      cairo_translate(cr, 0, -BLOCK_DIM / 2);
    else if(ftp == 2 || ftp == 3 || ftp == 4 || ftp == 5 || ftp == 6)
      cairo_translate(cr, BLOCK_DIM / 2, 0);
  }

  cairo_restore(cr);
  // -> PREVIEW End

  // ## STATISTIK
  cairo_save(cr);
  cairo_translate(cr, da_pos[2].x, da_pos[2].y);

  // Statistic Background
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_rectangle(cr, 0, 0, 10 * BLOCK_DIM + 1, -(22 * BLOCK_DIM + 1));
  cairo_fill(cr);

  static int stat_order[] = {0, 6, 4, 1, 5, 3, 2};

  for(k = 0; k < TYPES_CNT; k++) {
    f = &figures[stat_order[k]];
    bp.color = type_color_assoziation[f->type];

    // Draw complete block info
    cairo_save(cr);
    cairo_translate(cr, 3 * BLOCK_DIM, -(3 * k * BLOCK_DIM + 2 * BLOCK_DIM));

    // .. draw blocks
    cairo_save(cr);

    if(f->type == 0)
      cairo_translate(cr, 0, BLOCK_DIM / 2);
    else if(f->type == 2 || f->type == 3 || f->type == 4 || f->type == 5 || f->type == 6)
      cairo_translate(cr, -BLOCK_DIM / 2, 0);

    for(i = 0; i < 4; i++) {
      p = &f->blocks[i];
      draw_block(cr, &bp, p->x, p->y);
    }

    cairo_restore(cr);

    // .. write counts
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, BLOCK_DIM * 1.5);
    cairo_move_to(cr, 3 * BLOCK_DIM, 0);
    sprintf(stat_cnt_str, "%03d", stat_cnt[stat_order[k]]);
    cairo_show_text(cr, stat_cnt_str);
    cairo_restore(cr);
    // .. End draw complete block info
  }

  cairo_restore(cr);
  // -> STATISTIK End

  // ## INFO
  cairo_save(cr);
  cairo_translate(cr, da_pos[3].x, da_pos[3].y);

  // Info Background
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_rectangle(cr, 0, 0, 16 * BLOCK_DIM + 1, -(7 * BLOCK_DIM + 1));
  cairo_fill(cr);

  // write info
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, BLOCK_DIM * 1.5);

  // .. level
  int x2 = 0;
  x = BLOCK_DIM;
  x2 = 8 * BLOCK_DIM;
  y = -BLOCK_DIM;
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "Level:");
  sprintf(info_str, "%02d", level);
  cairo_move_to(cr, x2, y);
  cairo_show_text(cr, info_str);

  // .. rows
  y = -(3 * BLOCK_DIM);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "Rows:");
  sprintf(info_str, "%03d", rows);
  cairo_move_to(cr, x2, y);
  cairo_show_text(cr, info_str);

  // .. points
  y = -(5 * BLOCK_DIM);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, "Points:");
  sprintf(info_str, "%06d", points);
  cairo_move_to(cr, x2, y);
  cairo_show_text(cr, info_str);

  cairo_restore(cr);
  // .. End draw complete block info

  cairo_restore(cr);
  // -> STATISTIK End
}


static void
refresh()
{
  gtk_widget_queue_draw(darea);
}


static void
toggle_preview()
{
  preview_visible = !preview_visible;
  refresh();
}


static gboolean
clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
/*
  if (event->button == 1) {
    figure_new();
    refresh();
  }
*/

  return TRUE;
}


static int
check_figure(figure_t *f)
{
  int i = 0;
  point_t *b = NULL;
  point_t *p = NULL;

  p = &f->pos;
  for(i = 0; i < 4; i++) {
    b = &f->blocks[i];

    if(b->x + p->x < 0 || b->x + p->x >= AREA_WIDTH || b->y + p->y < 0 || area[b->x + p->x][b->y + p->y].fixed)
      return 1;
  }

  return 0;
}


static gboolean
close_game(gpointer user_data)
{
  if(roll_cnt < AREA_HEIGHT_EXT - 1) {
    roll_cnt++;
    refresh();
    return TRUE;
  } else {
    ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_GameOver.ogg", NULL);
    mode = STATE_GAME_FINISHED;
    return FALSE;
  }
}


static void
translate(int direction) // -1 = Left, 0 = Down, 1 = Right
{
  int i = 0;
  figure_t _f;
  point_t *b = NULL;
  point_t *p = NULL;
  int fixed = 0;
  int check_res = 0;

  memcpy(&_f, &f, sizeof(figure_t));

  switch(direction) {
    case -1:
      _f.pos.x -= 1;
    ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_PieceMoveLR.ogg", NULL);

      break;

    case 0:
    default:
      _f.pos.y -= 1;
      break;

    case 1:
      _f.pos.x += 1;
      ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_PieceMoveLR.ogg", NULL);
      break;
  }

  // Check and handle the result
  if(check_figure(&_f) == 0) {
    // Unset the blocks on the old position
    p = &f.pos;
    for(i = 0; i < 4; i++) {
      b = &f.blocks[i];
      area[b->x + p->x][b->y + p->y].set = 0;
    }

    // Set transformed and checked copy as original figure
    memcpy(&f, &_f, sizeof(figure_t));

    // Set the blocks on the new position
    for(i = 0; i < 4; i++) {
      b = &f.blocks[i];
      area[b->x + p->x][b->y + p->y].set = 1;
      area[b->x + p->x][b->y + p->y].color = type_color_assoziation[f.type];
    }
    refresh();
  } else {
    if(direction == 0) { // Collided on moving down
      key_down_cnt = 0;
      if(key_down_line >= f.pos.y) {
        points += (key_down_line - f.pos.y + 1);
        ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_HardDrop.ogg", NULL);
      } else {
        ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_PieceTouchDown.ogg", NULL);
      }

      if(figure_freeze(&f) == 0) {
        figure_new();
        key_down_line = f.pos.y;
        refresh();
        if(check_figure(&f)) {
          mode = STATE_GAME_CLOSING;
          g_source_remove(level_speed_id);
          level_speed_id = 0;
          g_timeout_add(100, close_game, NULL);
        }
      }
    }
  }
}


static void
rotate(int direction) // -1 = Left, 1 = Right
{
  int i = 0;
  int tmp = 0;
  figure_t _f;
  point_t *b = NULL;
  point_t *p = NULL;

  memcpy(&_f, &f, sizeof(figure_t));

  switch(_f.rotatable) {
    case 0:
      return;
      break;

    case 1:
      ;
      break;

    case 2:
    case 3:
      if(_f.rotation == 0) {
        if(_f.rotatable == 2)
          direction = 1;
        else
          direction = 2;
      } else {
        direction = -_f.rotation;
      }
      break;
  }

  switch(direction) {
    case -1:
      for(i = 0; i < 4; i++) {
        tmp = _f.blocks[i].x;
        _f.blocks[i].x = _f.blocks[i].y;
        _f.blocks[i].y = tmp;
        if(_f.blocks[i].x != 0)
          _f.blocks[i].x *= -1;
      }
      break;

    case 1:
    default:
      for(i = 0; i < 4; i++) {
        tmp = _f.blocks[i].x;
        _f.blocks[i].x = _f.blocks[i].y;
        _f.blocks[i].y = tmp;
        if(_f.blocks[i].y != 0)
          _f.blocks[i].y *= -1;
      }
      break;
  }

  // Check and handle the result
  if(check_figure(&_f) == 0) {
    _f.rotation += direction;
    // Unset the blocks on the old position
    p = &f.pos;
    for(i = 0; i < 4; i++) {
      b = &f.blocks[i];
      area[b->x + p->x][b->y + p->y].set = 0;
    }

    // Set transformed and checked copy as original figure
    memcpy(&f, &_f, sizeof(figure_t));

    // Set the blocks on the new position
    for(i = 0; i < 4; i++) {
      b = &f.blocks[i];
      area[b->x + p->x][b->y + p->y].set = 1;
      area[b->x + p->x][b->y + p->y].color = type_color_assoziation[f.type];
    }
    refresh();
  }

  ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_PieceRotateLR.ogg", NULL);
  ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_PieceMoveLR.ogg", NULL);
}


static gboolean
figure_auto_down(gpointer user_data)
{
  if(mode == STATE_GAME_PLAY && key_down == 0)
    translate(0);

  return TRUE;
}


static gboolean
input_handler(gpointer user_data)
{
  int coolup1 = 20;
  int coolup2 = 50;
  int factor = 5;

  if(mode == STATE_GAME_PLAY) {
    if(key_a) {
      if(key_a_cnt == 0 || (key_a_cnt >= coolup2 && key_a_cnt % factor == 0))
        rotate(1);
      key_a_cnt++;
    }

    if(key_b) {
      if(key_b_cnt == 0 || (key_b_cnt >= coolup2 && key_b_cnt % factor == 0))
        rotate(-1);
      key_b_cnt++;
    }

    if(key_down) {
      if(key_down_cnt == 0)
        set_level_speed(0);
      if(key_down_cnt == 0 || (key_down_cnt >= coolup1 && key_down_cnt % factor == 0))
        translate(0);
      if(key_down_cnt == 0)
        key_down_line = f.pos.y;
      key_down_cnt++;
    }

    if(key_left) {
      if(key_left_cnt == 0 || (key_left_cnt >= coolup1 && key_left_cnt % factor == 0))
        translate(-1);
      key_left_cnt++;
    }

    if(key_right) {
      if(key_right_cnt == 0 || (key_right_cnt >= coolup1 && key_right_cnt % factor == 0))
        translate(1);
      key_right_cnt++;
    }
  }

  return TRUE;
}


static void
set_level_speed(int stop)
{
  if(level_speed_id != 0)
    g_source_remove(level_speed_id);

  if(!stop)
    level_speed_id = g_timeout_add(1000 / (level + 1), figure_auto_down, NULL);
  else
    level_speed_id = 0;
}


static void
on_window_show (GtkWidget *widget, gpointer user_data)
{
  ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_Splash.ogg", NULL);
}


static void
set_window_size()
{
  ws.x = (30 + AREA_WIDTH ) * BLOCK_DIM;
  ws.y = ( 4 + AREA_HEIGHT) * BLOCK_DIM;

  GdkGeometry hints;
  hints.min_width = ws.x;
  hints.max_width = ws.x;
  hints.min_height = ws.y;
  hints.max_height = ws.y;

  gtk_window_set_geometry_hints(GTK_WINDOW(window), window, &hints, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}


static void
init_positions()
{
  // Area
  da_pos[0].x = 12 * BLOCK_DIM;
  da_pos[0].y = BLOCK_DIM + (AREA_HEIGHT_EXT * BLOCK_DIM + 1);

  // Preview
  da_pos[1].x = (19 + AREA_WIDTH) * BLOCK_DIM;
  da_pos[1].y = 14 * BLOCK_DIM + (4 * BLOCK_DIM + 1);

  // Statistic
  da_pos[2].x = BLOCK_DIM;
  da_pos[2].y = BLOCK_DIM + (22 * BLOCK_DIM + 1);

  // Info
  da_pos[3].x = (13 + AREA_WIDTH) * BLOCK_DIM;
  da_pos[3].y = 8 * BLOCK_DIM;
}


gboolean
on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  GdkModifierType modifiers;

  modifiers = gtk_accelerator_get_default_mod_mask();

  switch (event->keyval)
  {
    case GDK_f:
      if(mode == STATE_GAME_PLAY) {
        if(key_a == 0) {
          key_a = 1;
          key_a_cnt = 0;
        }
      }
      break;

    case GDK_d:
      if(mode == STATE_GAME_PLAY) {
        if(key_b == 0) {
          key_b = 1;
          key_b_cnt = 0;
        }
      }
      break;

    case GDK_s:
      if(mode != STATE_GAME_PAUSE)
        toggle_preview();
      break;

    case GDK_KEY_Return:
      switch(mode) {
        case STATE_INIT:
          break;

        case STATE_SELECT_LEVEL:
          preview_visible = 1;
          figure_new();
          refresh();
          level_preset = level;
          mode = STATE_GAME_PLAY;
          set_level_speed(0);
          static int input_handler_set = 0;
          if(!input_handler_set) {
            input_handler_set = 1;
            g_timeout_add(10, input_handler, NULL);
            ca_context_play (ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "sounds/SFX_GameStart.ogg", NULL);
          }

          break;

        case STATE_GAME_PLAY:
          mode = STATE_GAME_PAUSE;
          refresh();
          break;

        case STATE_GAME_PAUSE:
          mode = STATE_GAME_PLAY;
          refresh();
          break;

        case STATE_GAME_CLOSING:
        case STATE_GAME_FULL_ROW:
          ;
          break;

        case STATE_GAME_FINISHED:
          roll_cnt = 0;
          init_for_new_game();
          mode = STATE_SELECT_LEVEL;
          refresh();
          break;
      }
      break;

    case GDK_KEY_Up:
      if((event->state & modifiers) == GDK_CONTROL_MASK) {
        if(BLOCK_DIM < 30) {
          BLOCK_DIM++;
          set_window_size();
          init_positions();
          refresh();
        }
      } else {
        if(mode == STATE_SELECT_LEVEL) {
          if(level < 9) {
            level++;
            refresh();
          }
        }
      }
      break;

    case GDK_KEY_Down:
      if((event->state & modifiers) == GDK_CONTROL_MASK) {
        if(BLOCK_DIM > 8) {
          BLOCK_DIM--;
          set_window_size();
          init_positions();
          refresh();
       }
      } else {
        if(mode == STATE_SELECT_LEVEL) {
          if(level > 0) {
            level--;
            refresh();
          }
        } else if(mode == STATE_GAME_PLAY) {
          if(key_down == 0) {
            key_down = 1;
            key_down_cnt = 0;
          }
        }
      }
      break;

    case GDK_KEY_Left:
      if(mode ==  STATE_GAME_PLAY) {
        if(key_left == 0) {
          key_left = 1;
          key_left_cnt = 0;
        }
      }
      break;

    case GDK_KEY_Right:
      if(mode ==  STATE_GAME_PLAY) {
        if(key_right ==0 ) {
          key_right = 1;
          key_right_cnt = 0;
        }
      }
      break;

    default:
      return FALSE;
  }

  return FALSE;
}


gboolean
on_key_release(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  switch (event->keyval)
  {
    case GDK_f:
        key_a = 0;
        key_a_cnt = 0;
      break;

    case GDK_d:
        key_b = 0;
        key_b_cnt = 0;
      break;

    case GDK_KEY_Down:
        key_down = 0;
        key_down_cnt = 0;
        key_down_line = 0;
        break;

    case GDK_KEY_Left:
        key_left = 0;
        key_left_cnt = 0;
        break;

    case GDK_KEY_Right:
        key_right = 0;
        key_right_cnt = 0;
        break;

    default:
      return FALSE;
  }

  return FALSE;
}


int
main(int argc, char *argv[])
{
  mode = STATE_INIT;

  srand(time(NULL));

  create_colorsets();
  create_figures();

  ca_context_create (&ctx_sound);

  init_positions();
  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  darea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(window), darea);

  gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(darea,  "expose-event",       G_CALLBACK(on_expose_event), NULL);
  g_signal_connect(darea,  "button-press-event", G_CALLBACK(clicked), NULL);
  g_signal_connect(window, "show",               G_CALLBACK(on_window_show), NULL);
  g_signal_connect(window, "key-press-event",    G_CALLBACK(on_key_press), NULL);
  g_signal_connect(window, "key-release-event",  G_CALLBACK(on_key_release), NULL);
  g_signal_connect(window, "destroy",            G_CALLBACK(gtk_main_quit), NULL);


  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  set_window_size();
  gtk_window_set_title(GTK_WINDOW(window), "Tetris for my two beloved little ones (Bibis)");

  mode = STATE_SELECT_LEVEL;

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
