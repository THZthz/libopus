/**
 * @file vector_graphics.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/11/13
 *
 * @example
 *
 * @development_log
 *
 */

#include "engine/engine.h"
#include "render/color.h"
#include "utils/utils.h"

/**
 * @brief read shader code from a file(into `*str`). This function will allocate sufficient memory for it.
 * 		Also remember to call `free(*str)` to release its memory.
 */
static INLINE void read_file_into_string(const char *file_path, char **str)
{
	FILE *fp;
	long  file_size;
	char  err_msg[200];

	/* if we want to read whole file into memory in one time, we must open it in byte mode */
	if ((fp = fopen(file_path, "rb+")) == NULL) {
		sprintf(err_msg, "read_file_into_string: FAILED_TO_OPEN_FILE: %s", file_path);
		perror(err_msg);
		exit(1);
	}

	/* get the size of the file */
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	if ((*str = (char *) malloc(sizeof(char) * (file_size + 1))) == NULL) {
		sprintf(err_msg, "read_file_into_string: FAILED_TO_ALLOCATE_MEMORY: %s", file_path);
		perror(err_msg);
		exit(1);
	}

	/* TODO: check api of fread() */
	if (fread(*str, sizeof(char), file_size, fp) != file_size) {
		sprintf(err_msg, "read_file_into_string: FAILED_TO_READ_FILE: file path: %s %s", file_path, *str);
		perror(err_msg);
		exit(1);
	}

	fclose(fp);
}

/**
 * check compile status of shaders and program
 * @param shader
 * @param typ   type of the compile error you check, can only be "GL_VERTEX_SHADER", "GL_FRAGMENT_SHADER" or "-1"
 */
static INLINE int check_shader_error(GLuint shader, int type)
{
	int success;
	if (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER) {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			GLint info_len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
			if (info_len) {
				char *buf = (char *) malloc((size_t) info_len);
				if (buf) {
					glGetShaderInfoLog(shader, info_len, NULL, buf);
					ERROR_("SHADER_CREATE_ERROR %d:\n\t%s\n", type, buf);
					free(buf);
				}
				glDeleteShader(shader);
			}
		}
	} else {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success) {
			GLint info_len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
			if (info_len) {
				char *buf = (char *) malloc((size_t) info_len);
				if (buf) {
					glGetShaderInfoLog(shader, info_len, NULL, buf);
					ERROR_("SHADER_PROGRAM_LINKING_ERROR:\n\t%s\n", buf);
					free(buf);
				}
				glDeleteShader(shader);
			}
		}
	}
	return success;
}

static INLINE GLuint create_shaders_from_memory(const char *vertex_shader_source, const char *fragment_shader_source)
{
	int success, c = 0;

	/* compile shaders */
	GLuint vertex, fragment, program_id;

	/* vertex shader */
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex);
	check_shader_error(vertex, GL_VERTEX_SHADER);

	/* fragment Shader */
CREATE_FRAGMENT_SHADER:
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment);
	success = check_shader_error(fragment, GL_FRAGMENT_SHADER);
	/* FIXME: this is a nasty hack to fix a problem */
	if (!success && c == 0) {
		fragment_shader_source += 25;
		c = 1; /* let this piece of code only execute once */
		INFO("SHADER_CREATE_ERROR 35632 have been solved now.\n");
		goto CREATE_FRAGMENT_SHADER;
	}

	/* shader Program */
	program_id = glCreateProgram();
	glAttachShader(program_id, vertex);
	glAttachShader(program_id, fragment);
	glLinkProgram(program_id);
	check_shader_error(program_id, -1); /* -1 means not vertex shader nor fragment shader */

	/* delete the shaders as they're linked into our program now and no longer necessary */
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program_id;
}

static INLINE GLuint create_shaders_from_files(const char *vertex_shader_path, const char *fragment_shader_path)
{
	GLuint shader_id;

	/* retrieve the vertex/fragment source code from filePath */
	char *vertex_shader_source   = NULL;
	char *fragment_shader_source = NULL;
	read_file_into_string(vertex_shader_path, &vertex_shader_source);
	read_file_into_string(fragment_shader_path, &fragment_shader_source);
	shader_id = create_shaders_from_memory(vertex_shader_source, fragment_shader_source);
	free(vertex_shader_source);
	free(fragment_shader_source);

	return shader_id;
}

extern void vector_graphics_boot_plugin(engine_t *eng, plugin_t *plugin, ...)
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

	GLuint shader_program, VBO, VAO, EBO, texture;
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

	int width = eng->width;
	int height = eng->height;

	shader_program = create_shaders_from_memory(vertex_shader_source, fragment_shader_source_argb_premultiplied);

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

	/* generate texture, but only call GPU to allocate a memory place for it
	 * this is used to render what we draw using Plutovg on the screen
	 */
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture); /* all upcoming GL_TEXTURE_2D operations now have effect on this texture object */
		/* set the texture wrapping parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); /* set texture wrapping to GL_REPEAT (default wrapping method) */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		/* set texture filtering parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		/*glGenerateMipmap(GL_TEXTURE_2D);*/
	}

	/* some configuration about openGL */
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	eng->shader_program             = shader_program;
	eng->VAO                        = VAO;
	eng->VBO                        = VBO;
	eng->EBO                        = EBO;
	eng->texture                    = texture;
	eng->draw_data_to_current_frame = 1;
}

extern void vector_graphics_shutdown_plugin(engine_t* eng, plugin_t* plugin)
{
	/* properly de-allocate all resources once they've outlived their purpose */
	glDeleteVertexArrays(1, &eng->VAO);
	glDeleteBuffers(1, &eng->VBO);
	glDeleteBuffers(1, &eng->EBO);
	glDeleteProgram(eng->shader_program);
}
