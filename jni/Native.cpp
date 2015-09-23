#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* [Vertex source] */
static const char* VERTEX_SHADER = "attribute vec4 vPosition;"
		"attribute vec4 vTexCoordinate;"
		"uniform mat4 textureTransform;"
		"varying vec2 v_TexCoordinate;"

		"void main () {"
		"    v_TexCoordinate = (textureTransform*vTexCoordinate).xy;"
		"    gl_Position = vPosition;"
		"}";
/* [Vertex source] */

/* [Fragment source] */
static const char* FRAGMENT_SHADER =
		"#extension GL_OES_EGL_image_external : require\n"
				"precision mediump float;"
				"uniform samplerExternalOES texture;"
				"varying vec2 v_TexCoordinate;"

				"void main () {"
				"vec4 color = texture2D(texture, v_TexCoordinate);"
				"gl_FragColor = color;"
				"}";
/* [Fragment source] */

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

JNIEXPORT void JNICALL Java_dugu9sword_esplayer_VideoTextureSurfaceRenderer_nativeDrawTexture(
		JNIEnv* env, jobject obj) {

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, width, height);

	glUseProgram(program);
	int textureParamHandle = glGetUniformLocation(program, "texture");
	int textureCoordinateHandle = glGetAttribLocation(program,
			"vTexCoordinate");
	int positionHandle = glGetAttribLocation(program, "vPosition");
	int textureTranformHandle = glGetUniformLocation(program,
			"textureTransform");

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
			1.0f, 0.0f, 1.0f, -1.0f, 0.0f };

	glEnableVertexAttribArray(positionHandle);
	glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 4 * 3, vertices);

	glBindTexture(GL_TEXTURE0, textures[0]);
	glActiveTexture(GL_TEXTURE0);

	glUniform1i(textureParamHandle, 0);
	const GLfloat textureCoords[] = { 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f };
	glEnableVertexAttribArray(textureCoordinateHandle);
	glVertexAttribPointer(textureCoordinateHandle, 4, GL_FLOAT, false, 0,
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

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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
