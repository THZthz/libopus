/**
 * @file vg_gl.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/27
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef VG_GL_H
#define VG_GL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "vg/vg.h"
#include "math/math.h"
#include "external/glad/glad.h"
#include "external/STB/stb_truetype.h"

#define VG_GL_MAX_FONT_GLYPHS (256)

typedef struct opus_gl_program opus_gl_program;
typedef struct opus_font       opus_font;
typedef void (*opus_gl_program_init_cb)(opus_gl_program *program);
typedef void (*opus_gl_program_use_cb)(opus_gl_program *program, ...);
typedef void (*opus_gl_program_done_cb)(opus_gl_program *program);

struct opus_gl_program {
	GLuint program;
	GLuint vao, vbo;

	void *context_;

	opus_gl_program_init_cb init;
	opus_gl_program_use_cb  use;
	opus_gl_program_done_cb done;
};

struct opus_font {
	unsigned char *font_file;
	stbtt_fontinfo info;

	int ascent, descent, line_gap;
	int base_line;

	float scale, size;

	opus_real w, h;
	opus_real line_space;
};

void   vg_gl_read(const char *file_path, char **str);
int    vg_gl_check_shader_error(GLuint shader);
int    vg_gl_check_program_error(GLuint program);
GLuint vg_gl_create_program(const char *vertex_shader_source, const char *fragment_shader_source);
GLuint vg_gl_create_program_v(const char *vertex_shader_path, const char *fragment_shader_path);

opus_gl_program *vg_gl_program_create(opus_gl_program_init_cb init, opus_gl_program_use_cb use, opus_gl_program_done_cb done);
void             vg_gl_program_destroy(opus_gl_program *program);

opus_gl_program *vg_gl_program_preset1(void);
opus_gl_program *vg_gl_program_preset2(void);

void vg_gl_p1_render_fill(opus_gl_program *program, opus_vg *vg, double r, double g, double b, double a);
void vg_gl_p1_render_stroke(opus_gl_program *program, opus_vg *vg, double r, double g, double b, double a);

opus_font    *vg_gl_font_create(const char *font_file_path);
void          vg_gl_font_destroy(opus_font *font);

void vg_gl_font_set_size(opus_font *font, float size);
void vg_gl_font_set_bound(opus_font *font, opus_real w, opus_real h);
void vg_gl_font_set_line_space(opus_font *font, opus_real ratio);
void vg_gl_font_measure_text(opus_font *font, const char *words, size_t n, opus_real *ex_x,
                             opus_real *ex_y);
void vg_gl_font_generate_path(opus_font *font, opus_vg *vg, const char *words, size_t n,
                              opus_real x, opus_real y);
void vg_gl_font_fill(opus_font *font, opus_vg *vg, opus_gl_program *program);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VG_GL_H */
