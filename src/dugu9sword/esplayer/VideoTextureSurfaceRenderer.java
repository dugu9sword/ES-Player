package dugu9sword.esplayer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.*;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.util.Log;

public class VideoTextureSurfaceRenderer extends TextureSurfaceRenderer
		implements SurfaceTexture.OnFrameAvailableListener {

	public static final String TAG = VideoTextureSurfaceRenderer.class
			.getSimpleName();

	private Context context;

	private int[] textures = new int[1];

	private int shaderProgram;

	private SurfaceTexture surfaceTexture;
	private float[] videoTextureTransform;
	private boolean frameAvailable = false;

	public VideoTextureSurfaceRenderer(Context context, SurfaceTexture texture,
			int width, int height) {
		super(texture, width, height);
		this.context = context;
		videoTextureTransform = new float[16];
		surfaceTexture = new SurfaceTexture(textures[0]);
		surfaceTexture.setOnFrameAvailableListener(this);
	}

	@Override
	protected boolean draw() {
		synchronized (this) {
			if (frameAvailable) {
				surfaceTexture.updateTexImage();
				surfaceTexture.getTransformMatrix(videoTextureTransform);
				frameAvailable = false;
			} else {
				return false;
			}

		}
		this.nativeDrawTexture();
		return true;
	}
	
	private native void nativeDrawTexture();

	@Override
	protected void initGLComponents() {
		nativeSetupGraphics(width,height);
	}

	public native void nativeSetupGraphics(int width,int height);
	
	static{
		System.loadLibrary("Native");
	}

	@SuppressLint("NewApi")
	@Override
	protected void deinitGLComponents() {
		GLES20.glDeleteTextures(1, textures, 0);
		GLES20.glDeleteProgram(shaderProgram);
		surfaceTexture.release();
		surfaceTexture.setOnFrameAvailableListener(null);
	}

	@SuppressLint("NewApi")
	public void checkGlError(String op) {
		int error;
		while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
			Log.e("SurfaceTest",
					op + ": glError " + GLUtils.getEGLErrorString(error));
		}
	}

	@Override
	public SurfaceTexture getSurfaceTexture() {
		return surfaceTexture;
	}

	@Override
	public void onFrameAvailable(SurfaceTexture surfaceTexture) {
		synchronized (this) {
			frameAvailable = true;
		}
	}
}
