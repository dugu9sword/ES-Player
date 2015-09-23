#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "Matrix.h"

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* [Vertex source] */
static const char* VERTEX_SHADER = "attribute vec4 vPosition;"
		"attribute vec4 vTexCoordinate;"
		"uniform mat4 textureTransform;"
		"varying vec2 v_TexCoordinate;"
		"uniform mat4 projectionMatrix;"
		"uniform mat4 modelViewMatrix;"
		"void main () {\n"
		"    v_TexCoordinate = (textureTransform*vTexCoordinate).xy;"
		"    gl_Position = projectionMatrix * modelViewMatrix * vPosition;"
		"}";
/* [Vertex source] */

/* [Fragment source] */
static const char* FRAGMENT_SHADER =
		"#extension GL_OES_EGL_image_external : require\n"
				"precision mediump float;"
				"uniform samplerExternalOES texture;"
				"varying vec2 v_TexCoordinate;"

				"void main () {\n"
				"	vec4 color = texture2D(texture, v_TexCoordinate);"
				"	gl_FragColor = color;"
				"}";
/* [Fragment source] */

const GLfloat vertices[] = {-1.0f,  1.0f, -1.0f, /* Back. */
        1.0f,  1.0f, -1.0f,
       -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
       -1.0f,  1.0f,  1.0f, /* Front. */
        1.0f,  1.0f,  1.0f,
       -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
       -1.0f,  1.0f, -1.0f, /* Left. */
       -1.0f, -1.0f, -1.0f,
       -1.0f, -1.0f,  1.0f,
       -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f, /* Right. */
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
       -1.0f, 1.0f, -1.0f, /* Top. */
       -1.0f, 1.0f,  1.0f,
        1.0f, 1.0f,  1.0f,
        1.0f, 1.0f, -1.0f,
       -1.0f, - 1.0f, -1.0f, /* Bottom. */
       -1.0f,  -1.0f,  1.0f,
        1.0f, - 1.0f,  1.0f,
        1.0f,  -1.0f, -1.0f
      };

const GLfloat textureCoords[] = { 1.0f, 1.0f, 0.0f, /* Back. */
        0.0f, 1.0f,0.0f,
        1.0f, 0.0f,0.0f,
        0.0f, 0.0f,0.0f,
        0.0f, 1.0f, 0.0f,/* Front. */
        1.0f, 1.0f,0.0f,
        0.0f, 0.0f,0.0f,
        1.0f, 0.0f,0.0f,
        0.0f, 1.0f, 0.0f,/* Left. */
        0.0f, 0.0f,0.0f,
        1.0f, 0.0f,0.0f,
        1.0f, 1.0f,0.0f,
        1.0f, 1.0f, 0.0f,/* Right. */
        1.0f, 0.0f,0.0f,
        0.0f, 0.0f,0.0f,
        0.0f, 1.0f,0.0f,
        0.0f, 1.0f, 0.0f,/* Top. */
        0.0f, 0.0f,0.0f,
        1.0f, 0.0f,0.0f,
        1.0f, 1.0f,0.0f,
        0.0f, 0.0f, 0.0f,/* Bottom. */
        0.0f, 1.0f,0.0f,
        1.0f, 1.0f,0.0f,
        1.0f, 0.0f,0.0f,
};

GLushort indicies[] = {0, 3, 2, 0, 1, 3, 4, 6, 7, 4, 7, 5,  8, 9, 10, 8, 11, 10, 12, 13, 14, 15, 12, 14, 16, 17, 18, 16, 19, 18, 20, 21, 22, 20, 23, 22};


/* [loadShader] */
GLuint loadShader(GLenum shaderType, const char* shaderSource) {
	GLuint shader = glCreateShader(shaderType);
	if (shader) {
		glShaderSource(shader, 1, &shaderSource, NULL);
		glCompileShader(shader);

		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

			if (infoLen) {
				char * buf = (char*) malloc(infoLen);

				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					LOGE("Could not Compile Shader %d:\n%s\n", shaderType, buf);
					free(buf);
				}

				glDeleteShader(shader);
				shader = 0;
			}
		}
	}

	return shader;
}
/* [loadShader] */

/* [createProgram] */
GLuint createProgram(const char* vertexSource, const char * fragmentSource) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
	if (!vertexShader) {
		return 0;
	}

	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (!fragmentShader) {
		return 0;
	}

	GLuint program = glCreateProgram();

	if (program) {
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;

		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;

			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);

			if (bufLength) {
				char* buf = (char*) malloc(bufLength);

				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					LOGE("Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}

	return program;
}
/* [createProgram] */

GLuint program;
unsigned int* textures;
int width;
int height;

extern "C" {
JNIEXPORT void JNICALL Java_dugu9sword_esplayer_VideoTextureSurfaceRenderer_nativeDrawTexture(
		JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_dugu9sword_esplayer_VideoTextureSurfaceRenderer_nativeSetupGraphics(
		JNIEnv*, jobject, jint, jint);
}

JNIEXPORT void JNICALL setupTexture(JNIEnv*, jobject) {
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, textures);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);
}

JNIEXPORT void JNICALL loadShaders(JNIEnv* env, jobject obj) {
	program = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
	jclass clazz = env->GetObjectClass(obj);
	jfieldID shaderProgram = env->GetFieldID(clazz, "shaderProgram", "I");
	env->SetIntField(obj, shaderProgram, program);
}

int angle=0;

JNIEXPORT void JNICALL Java_dugu9sword_esplayer_VideoTextureSurfaceRenderer_nativeDrawTexture(
		JNIEnv* env, jobject obj) {

	glClearColor(1.0f, 1.0f, 1.0f, 0.5f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, width, height);

	glUseProgram(program);
	int textureParamHandle = glGetUniformLocation(program, "texture");
	int textureCoordinateHandle = glGetAttribLocation(program,
			"vTexCoordinate");
	int positionHandle = glGetAttribLocation(program, "vPosition");
	int textureTranformHandle = glGetUniformLocation(program,
			"textureTransform");
	int modelViewMatrixHandle = glGetUniformLocation(program,
				"modelViewMatrix");
	int projectionMatrixHandle = glGetUniformLocation(program,
				"projectionMatrix");

	float projectionMatrix[16];
	float modelViewMatrix[16];
	matrixPerspective(projectionMatrix, 45, (float)width / (float)height, 0.1f, 100);
	matrixIdentityFunction(modelViewMatrix);
    matrixRotateX(modelViewMatrix, ++angle);
    matrixRotateY(modelViewMatrix, ++angle);
	matrixTranslate(modelViewMatrix,0.0f,0.0f,-5.0f);

    glUniformMatrix4fv(projectionMatrixHandle, 1, GL_FALSE,projectionMatrix);
    glUniformMatrix4fv(modelViewMatrixHandle, 1, GL_FALSE, modelViewMatrix);


	glEnableVertexAttribArray(positionHandle);
	glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, vertices);

	glBindTexture(GL_TEXTURE0, textures[0]);
	glActiveTexture(GL_TEXTURE0);

	glUniform1i(textureParamHandle, 0);
	glEnableVertexAttribArray(textureCoordinateHandle);
	glVertexAttribPointer(textureCoordinateHandle, 3, GL_FLOAT, false, 0,
			textureCoords);

	jclass clazz = env->GetObjectClass(obj);
	jfieldID f_videoTextureTransform = env->GetFieldID(clazz,
			"videoTextureTransform", "[F");
	jfloatArray jfa_videoTextureTransform = (jfloatArray) env->GetObjectField(
			obj, f_videoTextureTransform);
	jfloat* v_videoTextureTransform = env->GetFloatArrayElements(
			jfa_videoTextureTransform, 0);
	glUniformMatrix4fv(textureTranformHandle, 1, false,
			v_videoTextureTransform);
	env->ReleaseFloatArrayElements(jfa_videoTextureTransform,
			v_videoTextureTransform, JNI_ABORT);

//	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indicies);
	glDisableVertexAttribArray(positionHandle);
	glDisableVertexAttribArray(textureCoordinateHandle);
}

JNIEXPORT void JNICALL Java_dugu9sword_esplayer_VideoTextureSurfaceRenderer_nativeSetupGraphics(
		JNIEnv* env, jobject obj, jint w, jint h) {
	width = w;
	height = h;

	/*
	 * 把java端的textures数组取过来
	 */
	jclass clazz = env->GetObjectClass(obj);
	jfieldID f_textures = env->GetFieldID(clazz,
			"textures", "[I");
	jintArray jia_textures = (jintArray) env->GetObjectField(
			obj, f_textures);
	textures = (unsigned int*)env->GetIntArrayElements(jia_textures, 0);


	setupTexture(env, obj);
	loadShaders(env, obj);
}
