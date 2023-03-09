/**
 * @file vg_gl.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/27
 *
 * @brief A fantastic website to learn openGL: http://www.glprogramming.com/red/index.html
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "math/math.h"
#include "utils/utils.h"
#include "vg/vg.h"
#include "vg/vg_gl.h"
#include "vg/vg_color.h"
#include "data_structure/array.h"
#include "external/STB/stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/STB/stb_image_write.h"

#include "vg/pluto/plutovg.h"
#include "vg/pluto/plutovg-private.h"

void vg_gl_read(const char *file_path, char **str)
{
	FILE *fp;
	long  file_size;

	*str = NULL;

	/* if we want to read whole file into memory in one time, we must open it in byte mode */
	if ((fp = fopen(file_path, "rb+")) == NULL) {
		OPUS_ERROR("read_file_into_string: FAILED_TO_OPEN_FILE: %s", file_path);
		return;
	}

	/* get the size of the file */
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	*str = (char *) calloc(file_size + 1, sizeof(char));
	if (!*str) {
		OPUS_ERROR("read_file_into_string: FAILED_TO_ALLOCATE_MEMORY: %s", file_path);
		return;
	}

	/* TODO: check api of fread() */
	if (fread(*str, sizeof(char), file_size, fp) != file_size) {
		OPUS_ERROR("read_file_into_string: FAILED_TO_READ_FILE: file path: %s %s", file_path, *str);
		return;
	}

	fclose(fp);
}

int vg_gl_check_shader_error(GLuint shader)
{
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
		if (info_len) {
			char *buf = (char *) malloc((size_t) info_len);
			if (buf) {
				glGetShaderInfoLog(shader, info_len, NULL, buf);
				OPUS_ERROR("SHADER_CREATE_ERROR:\n\t%s\n", buf);
				free(buf);
			}
			glDeleteShader(shader);
		}
	}
	return success;
}

int vg_gl_check_program_error(GLuint program)
{
	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLint info_len = 0;
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &info_len);
		if (info_len) {
			char *buf = (char *) malloc((size_t) info_len);
			if (buf) {
				glGetShaderInfoLog(program, info_len, NULL, buf);
				OPUS_ERROR("SHADER_PROGRAM_LINKING_ERROR:\n\t%s\n", buf);
				free(buf);
			}
			glDeleteShader(program);
		}
	}
	return success;
}

GLuint vg_gl_create_program(const char *vertex_shader_source, const char *fragment_shader_source)
{
	/* compile shaders */
	GLuint vertex, fragment, program_id;

	/* vertex shader */
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex);
	vg_gl_check_shader_error(vertex);

	/* fragment Shader */
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment);
	vg_gl_check_shader_error(vertex);

	/* shader Program */
	program_id = glCreateProgram();
	glAttachShader(program_id, vertex);
	glAttachShader(program_id, fragment);
	glLinkProgram(program_id);
	vg_gl_check_program_error(program_id);

	/* delete the shaders as they're linked into our program now and no longer necessary */
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program_id;
}

GLuint vg_gl_create_program_v(const char *vertex_shader_path, const char *fragment_shader_path)
{
	GLuint program;

	/* retrieve the vertex/fragment source code from filePath */
	char *vertex_shader_source   = NULL;
	char *fragment_shader_source = NULL;
	vg_gl_read(vertex_shader_path, &vertex_shader_source);
	vg_gl_read(fragment_shader_path, &fragment_shader_source);
	program = vg_gl_create_program(vertex_shader_source, fragment_shader_source);
	free(vertex_shader_source);
	free(fragment_shader_source);

	return program;
}

vg_gl_program_t *vg_gl_program_create(vg_gl_program_init_cb init, vg_gl_program_use_cb use, vg_gl_program_done_cb done)
{
	vg_gl_program_t *program;

	program = (vg_gl_program_t *) malloc(sizeof(vg_gl_program_t));
	OPUS_RETURN_IF(NULL, !program);

	program->program = -1;
	program->vao     = -1;
	program->vbo     = -1;

	program->init = init;
	program->use  = use;
	program->done = done;

	if (program->init) program->init(program);

	return program;
}

void vg_gl_program_destroy(vg_gl_program_t *program)
{
	if (program->done) program->done(program);
	free(program);
}

void preset1_init(vg_gl_program_t *program)
{
	const char *vertex =
	        "#version 330 core\n"
	        "layout (location = 0) in vec2 pos;\n"
	        "void main()\n"
	        "{\n"
	        "   gl_Position = vec4(pos.x / 400 - 1, -pos.y / 400 + 1, 0.0, 1.0);\n"
	        "}\0";

	const char *fragment =
	        "#version 330 core\n"
	        "in vec2 fpos;"
	        "out vec4 FragColor;\n"
	        "uniform vec4 color;\n"
	        "void main()\n"
	        "{\n"
	        "   FragColor = color;\n"
	        "}\n\0";

	program->program = vg_gl_create_program(vertex, fragment);

	glGenVertexArrays(1, &program->vao);
	glGenBuffers(1, &program->vbo);
	glBindVertexArray(program->vao);
	glBindBuffer(GL_ARRAY_BUFFER, program->vbo);

	glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, 2 * sizeof(double), (void *) 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void preset1_done(vg_gl_program_t *program)
{
	glDeleteVertexArrays(1, &program->vao);
	glDeleteBuffers(1, &program->vbo);
	glDeleteProgram(program->program);
}

/**
 * ARGS: vg_gl_program_t *program, GLenum mode, double *path, size_t n, double r, double g, double b, double a \n
 * NOTE: "n" is the number of double values in the path
 */
void preset1_use(vg_gl_program_t *program, ...)
{
	va_list args;
	GLenum  mode;
	double *path;
	int     n;
	double  r, g, b, a;

	va_start(args, program);
	mode = va_arg(args, GLenum);
	path = va_arg(args, double *);
	n    = va_arg(args, int);
	r    = va_arg(args, double);
	g    = va_arg(args, double);
	b    = va_arg(args, double);
	a    = va_arg(args, double);
	va_end(args);

	/*printf("use: %p %d %f %f %f %f\n", path, n, r, g, b, a);*/

	glUseProgram(program->program);
	glUniform4f(glGetUniformLocation(program->program, "color"), (float) r, (float) g, (float) b, (float) a);

	glBindVertexArray(program->vao);
	glBindBuffer(GL_ARRAY_BUFFER, program->vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (n * sizeof(double)), path, GL_STREAM_DRAW);
	glDrawArrays(mode, 0, (GLsizei) (n / 2));
}

/**
 * @brief specially designed to draw triangle strip data like [double, double, double, ...], \n
 * 		arguments of "use": vg_gl_program_t *program, GLenum mode, double *path, size_t n, double r, double g, double b, double a
 * @return
 */
vg_gl_program_t *vg_gl_program_preset1()
{
	return vg_gl_program_create(preset1_init, preset1_use, preset1_done);
}

struct context2 {
	GLuint program;
	GLuint vao, vbo, ebo;
	GLuint texture;

	int width, height;
};

void preset2_init(vg_gl_program_t *program)
{
	/* Shaders */
	/* FIXME: compatibility issue? I am tired of this. */
	const GLchar *vertex_shader_source =
	        /*"#version 330 core\n"*/
	        "attribute vec3 vertex_pos;\n"
	        "attribute vec3 vertex_color_in;\n"
	        "attribute vec2 texture_coord_in;\n"
	        "\n"
	        "varying vec3 vertex_color;\n"
	        "varying vec2 texture_coord;\n"
	        "\n"
	        "void main()\n"
	        "{\n"
	        "    gl_Position = vec4(vertex_pos, 1.0);\n"
	        "    vertex_color = vertex_color_in;\n"
	        "    texture_coord = vec2(texture_coord_in.x, texture_coord_in.y);\n"
	        "}";

	/**
	 * @brief shader for normal RGBA format data
	 */
	const GLchar *fragment_shader_source_rgba_normal =
	        /*"#version 330 core\n"*/
	        "precision mediump float;\n"
	        "varying vec3 vertex_color;\n"
	        "varying vec2 texture_coord;\n"
	        "\n"
	        "uniform sampler2D texture1;\n" /* texture sampler */
	        "\n"
	        "void main()\n"
	        "{\n"
	        "	vec4 c = texture(texture1, texture_coord);\n"
	        "	gl_FragColor = vec4(c.x, c.y, c.z, c.w) * vec4(vertex_color, 1.0f);\n" /* this is really complicated, I am crazy! */
	        "}\n";

	/**
	 * @brief this shader is specially for the image rendered by plutovg
	 */
	const GLchar *fragment_shader_source_argb_premultiplied =
	        /*"#version 330 core\n"*/
	        "precision mediump float;\n" /* some openGL versions does not support it */
	        "varying vec3 vertex_color;\n"
	        "varying vec2 texture_coord;\n"
	        "\n"
	        "uniform sampler2D texture1;\n" /* texture sampler */
	        "\n"
	        "void main()\n"
	        "{\n"
	        "	vec4 c = texture2D(texture1, texture_coord);\n"
	        "	float f1 = c.w, f2 = c.z, f3 = c.y, f4 = c.x;\n"
	        "	gl_FragColor = vec4(f2 / f1, f3 / f1, f4 / f1, f1) * vec4(vertex_color, 1.0);\n" /* for the cray format of plutovg */
	        "}\n";

	GLuint shader_program, VBO, VAO, EBO;
	float  vertices[] = {
            /* positions   colors       texture coords */
            1.f, 1.f, 0.0f, COLOR_SNOW, 1.0f, 0.0f,   /* top right */
            1.f, -1.f, 0.0f, COLOR_SNOW, 1.0f, 1.0f,  /* bottom right */
            -1.f, -1.f, 0.0f, COLOR_SNOW, 0.0f, 1.0f, /* bottom left */

            -1.f, 1.f, 0.0f, COLOR_SNOW, 0.0f, 0.0f,  /* top left */
            1.f, 1.f, 0.0f, COLOR_SNOW, 1.0f, 0.0f,   /* top right */
            -1.f, -1.f, 0.0f, COLOR_SNOW, 0.0f, 1.0f, /* bottom left */
    };
	unsigned int indices[] = {
	        0, 1, 2, /* first triangle */
	        3, 0, 2  /* second triangle */
	};

	struct context2 *context = (struct context2 *) malloc(sizeof(struct context2));
	OPUS_RETURN_IF(, !context);

	shader_program = vg_gl_create_program(vertex_shader_source, fragment_shader_source_argb_premultiplied);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	/* create who-know-what FIXME: I forget the name of those things */
	{
		GLint vp, vci, tci;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		/* position attribute */
		vp = glGetAttribLocation(shader_program, "vertex_pos");
		glVertexAttribPointer(vp, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
		glEnableVertexAttribArray(vp);
		/* color attribute */
		vci = glGetAttribLocation(shader_program, "vertex_color_in");
		glVertexAttribPointer(vci, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
		glEnableVertexAttribArray(vci);
		/* texture coord attribute */
		tci = glGetAttribLocation(shader_program, "texture_coord_in");
		glVertexAttribPointer(tci, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
		glEnableVertexAttribArray(tci);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	program->context_ = context;
	context->program  = shader_program;
	context->vao      = VAO;
	context->vbo      = VBO;
	context->ebo      = EBO;
	context->texture  = -1;

	context->width  = -1;
	context->height = -1;
}

/**
 * ARGS: vg_gl_program_t *program, unsigned char *data, int width, int height
 */
void preset2_use(vg_gl_program_t *program, ...)
{
	struct context2 *context = program->context_;

	va_list        args;
	int            width, height;
	unsigned char *data;

	OPUS_RETURN_IF(, !context);
	va_start(args, program);
	data   = va_arg(args, unsigned char *);
	width  = va_arg(args, int);
	height = va_arg(args, int);
	va_end(args);

	/* check if we haven't generate texture */
	if (context->texture == -1) {
		/* generate texture, but only call GPU to allocate a memory place for it
		 * this is used to render what we draw using pluto_vg on the screen
		 */
		{
			glGenTextures(1, &context->texture);
			glBindTexture(GL_TEXTURE_2D, context->texture); /* all upcoming GL_TEXTURE_2D operations now have effect on this texture object */
			/* set the texture wrapping parameters */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); /* set texture wrapping to GL_REPEAT (default wrapping method) */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			/* set texture filtering parameters */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			/*glGenerateMipmap(GL_TEXTURE_2D);*/
		}
	}

	/* check if the size of the texture is mis-matched */
	if (context->width != width || context->height != height) {
		context->width  = width;
		context->height = height;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glUseProgram(context->program);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* draw texture */
	{
		GLint t = glGetUniformLocation(context->program, "texture1");
		glBindTexture(GL_TEXTURE_2D, context->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(t, 0);
		glBindVertexArray(context->vao);
		/*glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);*/ /* FIXME: why this cannot work? */
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
}

void preset2_done(vg_gl_program_t *program)
{
	struct context2 *context = program->context_;
	OPUS_RETURN_IF(, !context);

	/* properly de-allocate all resources once they've outlived their purpose */
	glDeleteVertexArrays(1, &context->vao);
	glDeleteBuffers(1, &context->vbo);
	glDeleteBuffers(1, &context->ebo);
	if (context->texture != -1) glDeleteTextures(1, &context->texture);
	glDeleteProgram(context->program);

	free(context);
}

/**
 * @brief used to draw pluto_vg surface buffer, \n
 * 		ARGS: vg_gl_program_t *program, unsigned char *data, int width, int height
 * @return
 */
vg_gl_program_t *vg_gl_program_preset2()
{
	return vg_gl_program_create(preset2_init, preset2_use, preset2_done);
}

/**
 * @brief Use stencil buffer to fill a concave polygon (or convex of course).
 * 		The detail of this method can be found at http://glprogramming.com/red/chapter14.html#name13 (nice!)
 * @param program preset1, use it! That's why this function have "_p1_"
 */
void vg_gl_p1_render_fill(vg_gl_program_t *program, opus_vg *vg, double r, double g, double b, double a)
{
	size_t j;

	glUseProgram(program->program);

	glEnable(GL_STENCIL_TEST);
	for (j = 0; j < opus_arr_len(vg->paths); j++) {
		opus_vg_path *path = &vg->paths[j];

		if (path->n_fill == 0) continue;

		/* fill stencil buffer */
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
		glStencilFunc(GL_ALWAYS, 0x1, 0x1);
		program->use(program, GL_TRIANGLE_FAN, path->fill, path->n_fill * 2, r, g, b, a);
	}

	for (j = 0; j < opus_arr_len(vg->paths); j++) {
		opus_vg_path *path = &vg->paths[j];

		if (path->n_fill == 0) continue;

		/* fill color buffer */
		glColor3ub(0, 128, 0);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilFunc(GL_EQUAL, 0x1, 0x1);
		program->use(program, GL_TRIANGLE_FAN, path->fill, path->n_fill * 2, r, g, b, a);
	}
	glDisable(GL_STENCIL_TEST);
}

/**
 * @brief Use preset1 vg_gl_program to render stroke
 * @param program a preset1 vg_gl_program
 */
void vg_gl_p1_render_stroke(vg_gl_program_t *program, opus_vg *vg, double r, double g, double b, double a)
{
	size_t i;

	glUseProgram(program->program);
	glUniform4f(glGetUniformLocation(program->program, "color"), (float) r, (float) g, (float) b, (float) a);

	glBindVertexArray(program->vao);
	glBindBuffer(GL_ARRAY_BUFFER, program->vbo);

	for (i = 0; i < opus_arr_len(vg->paths); i++) {
		opus_vg_path *path = &vg->paths[i];
		size_t     n    = path->n_stroke;

		if (n == 0) continue;

		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (n * 2 * sizeof(double)), (double *) path->stroke, GL_STREAM_DRAW);
		/* glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei) (n)); */

		/* glEnable(GL_BLEND); */
		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xff);

		/* fill the stroke base without overlap */
		glStencilFunc(GL_EQUAL, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei) n);

		/* draw anti-aliased pixels */
		glStencilFunc(GL_EQUAL, 0x00, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei) n);

		/* clear stencil buffer */
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilFunc(GL_ALWAYS, 0x0, 0xff);
		glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei) n);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glDisable(GL_STENCIL_TEST);
		/* glDisable(GL_BLEND); */
	}
	/* program->use(program, GL_TRIANGLE_STRIP, path->stroke, path->n_stroke * 2, r, g, b, a); */
}

vg_gl_font_t *vg_gl_font_create(const char *font_file_path)
{
	vg_gl_font_t *font = (vg_gl_font_t *) malloc(sizeof(vg_gl_font_t));

	OPUS_RETURN_IF(NULL, !font);

	vg_gl_read(font_file_path, (char **) &font->font_file);
	if (!stbtt_InitFont(&font->info, font->font_file, 0)) {
		OPUS_ERROR("vg_gl_font_create::Failed to init STB true type library\n");
		free(font->font_file);
		free(font);
		return NULL;
	}

	vg_gl_font_set_bound(font, 0, 0);
	font->line_space = 1;

	stbtt_GetFontVMetrics(&font->info, &font->ascent, &font->descent, &font->line_gap);
	font->descent = opus_abs(font->descent);

	vg_gl_font_set_size(font, 100);

	return font;
}

void vg_gl_font_set_size(vg_gl_font_t *font, float size)
{
	font->size      = size;
	font->scale     = stbtt_ScaleForPixelHeight(&font->info, font->size);
	font->base_line = (int) ((float) font->ascent * font->scale);
	font->ascent    = (int) ((float) font->ascent * font->scale);
	font->descent   = (int) ((float) font->descent * font->scale);
	font->line_gap  = (int) ((float) font->line_gap * font->scale);
}

void vg_gl_font_destroy(vg_gl_font_t *font)
{
	if (font) {
		free(font->font_file);
		free(font);
	}
}

/* set 0 to leave the right to program(unlimited width and height) */
void vg_gl_font_set_bound(vg_gl_font_t *font, opus_real w, opus_real h)
{
	font->w = w;
	font->h = h;
}

/**
 * @brief line space will be calculated like this:
 * 		"next line's starting Y" = "previous line's starting Y" + "next line's height" * ratio
 * @param font
 * @param ratio
 */
void vg_gl_font_set_line_space(vg_gl_font_t *font, opus_real ratio)
{
	font->line_space = ratio;
}

void vg_gl_font_fill(vg_gl_font_t *font, opus_vg *vg, vg_gl_program_t *program)
{
	/* because we apply global transform on the path of the text glyph */
	/* we need to compensate for it */
	vg->line_width /= font->scale;
	opus_vg_end_fill_path(vg);
	vg->line_width *= font->scale;

	vg_gl_p1_render_fill(program, vg, COLOR_BLACK, 1);
}

void vg_gl_font_measure_text(vg_gl_font_t *font, const char *words, size_t n, opus_real *ex_x,
                             opus_real *ex_y)
{
#define MOVE_TO_NEXT_LINE()                 \
	do {                                    \
		y += font->line_space * font->size; \
		x             = ox;                 \
		total_advance = 0;                  \
	} while (0)

	size_t i;
	int    total_advance = 0;

	opus_real x = 0, y = 0, ox = x, oy = y;

	*ex_x = -1;
	*ex_y = -1;
	for (i = 0; i < n; i++) {
		char word = words[i];

		stbtt_vertex *v;

		int advance;
		int real_advance;
		int x1, y1, x2, y2;
		int index, n_v;

		/* check if Y is out of bound */
		if (font->h > 0 && y + font->size > oy + font->h) break;

		/* check for line break */
		if (word == '\n') {
			MOVE_TO_NEXT_LINE();
			continue; /* omit this special char */
		}

		if ((int) word > VG_GL_MAX_FONT_GLYPHS) {
			OPUS_ERROR("vg_gl_font_generate_path::Cannot render word if it is greater than 256: %c(%d)\n", word, (int) word);
			*ex_x = -1;
			*ex_y = -1;
			return;
		}

		/* get glyph's path */
		index = stbtt_FindGlyphIndex(&font->info, word);
		n_v   = stbtt_GetGlyphShape(&font->info, index, &v);

		/* get character' advance and bounding box information */
		stbtt_GetGlyphHMetrics(&font->info, index, &advance, NULL);
		real_advance = (int) ((float) advance * font->scale);

		/* check if the text is out of bound when we add this character */
		if (font->w > 0 && real_advance < font->w) {
			if (total_advance + real_advance > font->w) MOVE_TO_NEXT_LINE();
		}

		total_advance += (int) ((float) advance * font->scale);
		if (total_advance > *ex_x) *ex_x = total_advance + (int) font->size / 2; /* FIXME */

		stbtt_FreeShape(&font->info, v); /* TODO: we need cache glyph ? */
	}

	*ex_y = y + font->size; /* FIXME */
}

/**
 * @brief
 * @param font
 * @param vg
 * @param program should be preset1
 * @param words
 * @param n
 * @param x
 * @param y
 */
void vg_gl_font_generate_path(vg_gl_font_t *font, opus_vg *vg, const char *words, size_t n,
                              opus_real x, opus_real y)
{
#define MOVE_TO_NEXT_LINE()                 \
	do {                                    \
		y += font->line_space * font->size; \
		x             = ox;                 \
		total_advance = 0;                  \
	} while (0)

	size_t i, j;
	int    total_advance = 0;

	opus_real ox = x, oy = y;

	for (i = 0; i < n; i++) {
		char word = words[i];

		stbtt_vertex *v;

		int advance;
		int real_advance;
		int x1, y1, x2, y2;
		int index, n_v;

		/* check if Y is out of bound */
		if (y + font->size > oy + font->h) break;

		/* check for line break */
		if (word == '\n') {
			MOVE_TO_NEXT_LINE();
			continue; /* omit this special char */
		}

		if ((int) word > VG_GL_MAX_FONT_GLYPHS) {
			OPUS_ERROR("vg_gl_font_generate_path::Cannot render word if it is greater than 256: %c(%d)\n", word, (int) word);
			return;
		}

		/* get glyph's path */
		index = stbtt_FindGlyphIndex(&font->info, word);
		n_v   = stbtt_GetGlyphShape(&font->info, index, &v);

		/* get character' advance and bounding box information */
		stbtt_GetGlyphHMetrics(&font->info, index, &advance, NULL);
		stbtt_GetGlyphBox(&font->info, index, &x1, &y1, &x2, &y2);
		real_advance = (int) ((float) advance * font->scale);

		/* check if the text is out of bound when we add this character */
		if (font->w > 0 && real_advance < font->w) {
			if (total_advance + real_advance > font->w) MOVE_TO_NEXT_LINE();
		}

		/* draw glyph */
		opus_vg_save(vg);
		opus_vg_translate(vg, (float) x + (float) total_advance, (float) y);
		opus_vg_scale(vg, font->scale, -font->scale);
		for (j = 0; j < n_v; j++) {
			switch (v[j].type) {
				case STBTT_vmove:
					opus_vg_move_to(vg, v[j].x, v[j].y);
					break;
				case STBTT_vline:
					opus_vg_line_to(vg, v[j].x, v[j].y);
					break;
				case STBTT_vcurve:
					opus_vg_quad_to(vg, v[j].cx, v[j].cy, v[j].x, v[j].y);
					break;
				case STBTT_vcubic:
					opus_vg_cubic_to(vg, v[j].cx, v[j].cy, v[j].cx1, v[j].cy1, v[j].x, v[j].y);
					break;
			}
		}
		/* render outline of the character */
		/*vg_rect(vg, x1, y1, x2 - x1, y2 - y1);*/

		opus_vg_restore(vg);

		total_advance += (int) ((float) advance * font->scale);

		stbtt_FreeShape(&font->info, v); /* TODO: we need cache glyph ? */
	}
}
