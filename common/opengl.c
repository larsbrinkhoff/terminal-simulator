#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifndef SHADERDIR
# define SHADERDIR ""
#endif

static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLVALIDATEPROGRAMPROC glValidateProgram;
static PFNGLGETPROGRAMIVPROC glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM1FPROC glUniform1f;
static PFNGLUNIFORM2FPROC glUniform2f;
static GLuint shader;

static SDL_bool init_proc (void) {
  glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
  glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
  glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
  glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
  glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
  glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
  glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
  glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
  glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
  glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
  glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
  glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
  glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
  glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
  glUniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
  glUniform2f = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
  
  return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && 
    glGetShaderInfoLog && glDeleteShader && glAttachShader && glCreateProgram &&
    glLinkProgram && glValidateProgram && glGetProgramiv && glGetProgramInfoLog &&
    glUseProgram;
}

static GLuint compile (const char *file, GLenum type)
{
  FILE *f;
  size_t len;
  GLuint shader;
  char *src;
  int success;

  f = fopen (file, "r");
  if (f == NULL) {
    fprintf (stderr, "failed to open shader: %s\n", file);
    return 0;
  }
  fseek (f, 0, SEEK_END);
  len = ftell (f);
  fseek (f, 0, SEEK_SET);
  src = malloc (len + 1);

  fread (src, 1, len, f);
  src[len] = 0;

  shader = glCreateShader (type);
  glShaderSource (shader, 1, (const char**)&src, 0);
  free (src);

  glCompileShader (shader);
  glGetShaderiv (shader, GL_OBJECT_COMPILE_STATUS_ARB, &success);
  if (!success) {
    int info_len;
    char *info_log;
		
    glGetShaderiv (shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_len);
    if (info_len > 0) {
      if (!(info_log = malloc(info_len + 1))) {
        perror ("malloc failed");
        return 0;
      }
      glGetShaderInfoLog (shader, info_len, 0, info_log);
      fprintf (stderr, "shader compilation failed: %s\n", info_log);
      free (info_log);
    } else {
      fprintf (stderr, "shader compilation failed\n");
    }
    return 0;
  }
  return shader;
}

static GLuint init_shader (void)
{
  unsigned int prog;
  int shader1, shader2, linked;

  shader1 = compile (SHADERDIR "vertex.shader", GL_VERTEX_SHADER);
  shader2 = compile (SHADERDIR "crt.shader", GL_FRAGMENT_SHADER);

  prog = glCreateProgram ();
  glAttachShader (prog, shader1);
  glAttachShader (prog, shader2);
  glLinkProgram (prog);
  glGetProgramiv (prog, GL_OBJECT_LINK_STATUS_ARB, &linked);
  if (!linked) {
    fprintf (stderr, "shader linking failed\n");
    return 0;
  }

  return prog;
}

static void set_uniform1f (unsigned int prog, const char *name, float val) {
  int loc = glGetUniformLocation (prog, name);
  if(loc != -1)
    glUniform1f (loc, val);
}

static void set_uniform2f (unsigned int prog, const char *name,
			   float val1, float val2) {
  int loc = glGetUniformLocation (prog, name);
  if(loc != -1)
    glUniform2f (loc, val1, val2);
}

void opengl_present (SDL_Texture *target, float width, float height)
{
  extern float curvature;

  SDL_GL_BindTexture (target, NULL, NULL);
  glUseProgram (shader);
  set_uniform2f (shader, "resolution", width, height);
  set_uniform1f (shader, "curvature", curvature);
  glViewport (0, 0, width, height);
  glBegin (GL_TRIANGLE_STRIP);
  glVertex2f (-width, -height);
  glVertex2f (width, -height);
  glVertex2f (-width, height);
  glVertex2f (width, height);
  glEnd ();
}

void init_opengl (void)
{
  init_proc ();
  shader = init_shader ();
}
