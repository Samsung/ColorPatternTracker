package com.samsung.dtl.colorpatterntracker.camera;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import android.content.Context;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES31;
import android.os.Environment;
import android.util.Log;

import javax.microedition.khronos.opengles.GL10;

// TODO: Auto-generated Javadoc
/**
 * The Class ShaderManager.
 */
public class ShaderManager {

	public int[] hTex; /*!< The texture to store camera image. */
	//public int[] glTextures; /*!< The input and output opengl textures. */
	public int[] glTextures_in; /*!< The input opengl textures. */
	public int[] glTextures_out; /*!< The input opengl textures. */
	public IntBuffer[] targetFramebuffer; /*!< The target framebuffer. */

	private FloatBuffer vertexCoord; /*!< The vertex coordinates. */
	private FloatBuffer cameraTexCoord; /*!< The camera texture coordinates. */
	private FloatBuffer openclTexCoord; /*!< The opencl texture coordinates. */

	private int hProgram; /*!< The shader program to get camera texture. */
	private int displayTextureProgram; /*!< The shader program to display texture. */

	/** The vertex shader source. */
	private final String vss =
			"attribute vec2 vPosition;\n" +
					"attribute vec2 vTexCoord;\n" +
					"varying vec2 texCoord;\n" +
					"void main() {\n" +
					"  texCoord = vTexCoord;\n" +
					"  gl_Position = vec4 ( vPosition.x, vPosition.y, 0.0, 1.0 );\n" +
					"}";

	/** The camera texture shader source. */
	private final String camera_fss =
			"#extension GL_OES_EGL_image_external : require\n" +
					"precision mediump float;\n" +
					"uniform samplerExternalOES sTexture;\n" +
					"varying vec2 texCoord;\n" +
					"void main() {\n" +
					"  gl_FragColor = texture2D(sTexture, texCoord);\n" +
					"}";

	/** The texture shader source. */
	private final String texture_fss =
			"precision mediump float;\n" +
					"uniform sampler2D sTexture;\n" +
					"varying vec2 texCoord;\n" +
					"void main() {\n" +
					"  gl_FragColor = texture2D(sTexture, texCoord);\n" +
					"}";

	public Context context;

	/**
	 * Initialize texture coordinates.
	 */
	public void initializeCoords(){
		float[] vertexCoordTmp = {
				-1.0f, -1.0f, 0.0f,
				-1.0f,  1.0f, 0.0f,
				1.0f, -1.0f, 0.0f,
				1.0f,  1.0f, 0.0f};
		float[] textureCoordTmp = {
				0.0f, 1.0f,
				0.0f, 0.0f,
				1.0f, 1.0f,
				1.0f, 0.0f };
		float[] openclCoordTmp = {
				0.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 0.0f,
				1.0f, 1.0f
		};

		vertexCoord = ByteBuffer.allocateDirect(12*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
		vertexCoord.put(vertexCoordTmp);
		vertexCoord.position(0);
		cameraTexCoord = ByteBuffer.allocateDirect(8*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
		cameraTexCoord.put(textureCoordTmp);
		cameraTexCoord.position(0);
		openclTexCoord = ByteBuffer.allocateDirect(8*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
		openclTexCoord.put(openclCoordTmp);
		openclTexCoord.position(0);
	}

	/**
	 * Get texture from Camera.
	 *
	 * @param mSTexture the camera surface texture
	 * @param camera_res the camera resolution
	 * @return the capture time
	 */
	public long cameraToTexture(SurfaceTexture mSTexture, Point camera_res, int frameBufferID, int nFBOs, boolean saveFiles) {
		long captureTime=0;
		int error0 = GLES31.glGetError();
		GLES31.glBindFramebuffer(GLES31.GL_FRAMEBUFFER, targetFramebuffer[frameBufferID].get(0));
		int error1 = GLES31.glGetError();
		int fbret = GLES31.glCheckFramebufferStatus(GLES31.GL_FRAMEBUFFER);
		int error2 = GLES31.glGetError();
		if (fbret != GLES31.GL_FRAMEBUFFER_COMPLETE) {
			Log.d("", "unable to bind fbo" + fbret);
		}
		GLES31.glViewport(0, 0, camera_res.x, camera_res.y);
		int error3 = GLES31.glGetError();
		synchronized(this) {
			mSTexture.updateTexImage();
			captureTime=mSTexture.getTimestamp();
		}

		GLES31.glUseProgram(hProgram);
		int error4 = GLES31.glGetError();

		int ph = GLES31.glGetAttribLocation(hProgram, "vPosition");
		int tch = GLES31.glGetAttribLocation (hProgram, "vTexCoord");
		int th = GLES31.glGetUniformLocation (hProgram, "sTexture");

		GLES31.glActiveTexture(GLES31.GL_TEXTURE0);
		int error5 = GLES31.glGetError();
		GLES31.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, hTex[0]);
		int error6 = GLES31.glGetError();
		GLES31.glUniform1i(th, 0);
		int error7 = GLES31.glGetError();

		GLES31.glVertexAttribPointer(ph, 2, GLES31.GL_FLOAT, false, 4*3, vertexCoord);
		int error8 = GLES31.glGetError();
		GLES31.glVertexAttribPointer(tch, 2, GLES31.GL_FLOAT, false, 4*2, cameraTexCoord);
		int error9 = GLES31.glGetError();
		GLES31.glEnableVertexAttribArray(ph);
		int error10 = GLES31.glGetError();
		GLES31.glEnableVertexAttribArray(tch);
		int error11 = GLES31.glGetError();

		GLES31.glDrawArrays(GLES31.GL_TRIANGLE_STRIP, 0, 4);
		int error12 = GLES31.glGetError();
		GLES31.glFinish();

		if(saveFiles) {
			for (int i = 0; i < nFBOs; i++) {
				GLES31.glBindFramebuffer(GLES31.GL_FRAMEBUFFER, targetFramebuffer[i].get(0));
				ByteBuffer byteBuffer = ByteBuffer.allocate(1920 * 1080 * 4);
				byteBuffer.order(ByteOrder.nativeOrder());

				GLES31.glReadPixels(0, 0, 1920, 1080, GLES31.GL_RGBA, GLES31.GL_UNSIGNED_BYTE, byteBuffer);
				int error14 = GLES31.glGetError();
				byte[] array = byteBuffer.array();

				java.io.FileOutputStream outputStream = null;
				try {
					String diskstate = Environment.getExternalStorageState();
					if (diskstate.equals("mounted")) {
						java.io.File picFolder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
						java.io.File picFile = new java.io.File(picFolder, "imgc" + i + ".bin");
						outputStream = new java.io.FileOutputStream(picFile);
					}
				} catch (FileNotFoundException e) {
					e.printStackTrace();
				}

				try {
					outputStream.write(array);
				} catch (IOException e) {
					e.printStackTrace();
				}

				try {
					outputStream.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}

		return captureTime;
	}

	/**
	 * Renders screen from texture.
	 *
	 * @param displayTexture the display texture
	 * @param display_dim the display dimension
	 */
	public void renderFromTexture(int displayTexture, Point display_dim) {
		GLES31.glViewport(0, 0, display_dim.x, display_dim.y);

		GLES31.glBindFramebuffer(GLES31.GL_FRAMEBUFFER, 0);
		GLES31.glUseProgram(displayTextureProgram);

		int ph = GLES31.glGetAttribLocation(displayTextureProgram, "vPosition");
		int tch = GLES31.glGetAttribLocation(displayTextureProgram, "vTexCoord");
		int th = GLES31.glGetUniformLocation(displayTextureProgram, "sTexture");

		GLES31.glActiveTexture(GLES31.GL_TEXTURE0);
		GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, displayTexture);
		GLES31.glUniform1i(th, 0);

		GLES31.glVertexAttribPointer(ph, 2, GLES31.GL_FLOAT, false, 4*3, vertexCoord);
		GLES31.glVertexAttribPointer(tch, 2, GLES31.GL_FLOAT, true, 4*2, openclTexCoord);
		GLES31.glEnableVertexAttribArray(ph);
		GLES31.glEnableVertexAttribArray(tch);
		GLES31.glDrawArrays(GLES31.GL_TRIANGLE_STRIP, 0, 4);
		GLES31.glFinish();
	}

	/**
	 * Initializes the textures.
	 *
	 * @param camera_res the camera resolution
	 * @return the surface texture
	 */
	public SurfaceTexture initTex(Point camera_res, int nFBOs) {
		hTex = new int[1];
		int [] glTextures_temp = new int[1];
		int error = GLES31.glGetError();
		GLES31.glGenTextures ( 1, hTex, 0 );error = GLES31.glGetError();
		GLES31.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, hTex[0]);error = GLES31.glGetError();
		GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES31.GL_TEXTURE_WRAP_S, GLES31.GL_CLAMP_TO_EDGE);error = GLES31.glGetError();
		GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES31.GL_TEXTURE_WRAP_T, GLES31.GL_CLAMP_TO_EDGE);error = GLES31.glGetError();
		//GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES31.GL_TEXTURE_MIN_FILTER, GLES31.GL_LINEAR_MIPMAP_LINEAR);error = GLES31.glGetError();
		//GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES31.GL_TEXTURE_MIN_FILTER, GLES31.GL_LINEAR);error = GLES31.glGetError();
		//GLES31.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES31.GL_TEXTURE_MAG_FILTER, GLES31.GL_LINEAR);error = GLES31.glGetError();
		glTextures_in = new int[nFBOs];
		glTextures_out = new int[1];
		for(int i=0;i<nFBOs;i++) {
			GLES31.glGenTextures ( 1, glTextures_temp, 0 );error = GLES31.glGetError();
			glTextures_in[i] = glTextures_temp[0];

			GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, glTextures_in[i]);error = GLES31.glGetError();
			GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D,GLES31.GL_TEXTURE_MIN_FILTER,GLES31.GL_LINEAR_MIPMAP_NEAREST);error = GLES31.glGetError();
			GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D,GLES31.GL_TEXTURE_MAG_FILTER,GLES31.GL_LINEAR);error = GLES31.glGetError();
			//GLES31.glTexImage2D(GLES31.GL_TEXTURE_2D, 0, GLES31.GL_RGBA, camera_res.x, camera_res.y, 0, GLES31.GL_RGBA, GLES31.GL_UNSIGNED_BYTE, null);error = GLES31.glGetError();
			GLES31.glTexStorage2D(GLES31.GL_TEXTURE_2D, 1, GLES31.GL_RGBA8, camera_res.x, camera_res.y);error = GLES31.glGetError();
			GLES31.glGenerateMipmap(GLES31.GL_TEXTURE_2D);error = GLES31.glGetError();
			GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, 0);error = GLES31.glGetError();
			//GLES31.glActiveTexture(GLES31.GL_TEXTURE0);
			//GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, glTextures[0]);error = GLES31.glGetError();
			GLES31.glBindImageTexture(0, glTextures_in[i], 0,true,0,GLES31.GL_READ_ONLY,GLES31.GL_RGBA8);
			error = GLES31.glGetError();
		}
		
		GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, glTextures_out[0]);error = GLES31.glGetError();
		GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D,GLES31.GL_TEXTURE_MIN_FILTER,GLES31.GL_LINEAR);error = GLES31.glGetError();
		GLES31.glTexParameteri(GLES31.GL_TEXTURE_2D,GLES31.GL_TEXTURE_MAG_FILTER,GLES31.GL_LINEAR);error = GLES31.glGetError();
		//GLES31.glTexImage2D(GLES31.GL_TEXTURE_2D, 0, GLES31.GL_RGBA, camera_res.x, camera_res.y, 0, GLES31.GL_RGBA, GLES31.GL_UNSIGNED_BYTE, null);error = GLES31.glGetError();
		GLES31.glTexStorage2D(GLES31.GL_TEXTURE_2D, 1, GLES31.GL_RGBA8, camera_res.x, camera_res.y);error = GLES31.glGetError();
		GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, 0);error = GLES31.glGetError();
		//GLES31.glActiveTexture(GLES31.GL_TEXTURE1);
		//GLES31.glBindTexture(GLES31.GL_TEXTURE_2D, glTextures[1]);error = GLES31.glGetError();
		GLES31.glBindImageTexture(1, glTextures_out[0], 0, true,0,GLES31.GL_WRITE_ONLY,GLES31.GL_RGBA8);
		error = GLES31.glGetError();

		targetFramebuffer = new IntBuffer[nFBOs];
		error = GLES31.glGetError();
		for(int i=0;i<nFBOs;i++) {
			targetFramebuffer[i] = IntBuffer.allocate(1);error = GLES31.glGetError();
			GLES31.glGenFramebuffers(1, targetFramebuffer[i]);error = GLES31.glGetError();
			GLES31.glBindFramebuffer(GLES31.GL_FRAMEBUFFER, targetFramebuffer[i].get(0));error = GLES31.glGetError();
			GLES31.glFramebufferTexture2D(GLES31.GL_FRAMEBUFFER, GLES31.GL_COLOR_ATTACHMENT0, GLES31.GL_TEXTURE_2D, glTextures_in[i], 0);error = GLES31.glGetError();
			GLES31.glBindFramebuffer(GLES31.GL_FRAMEBUFFER, 0);error = GLES31.glGetError();
		}

		GLES31.glClearColor (1.0f, 1.0f, 0.0f, 1.0f);error = GLES31.glGetError();
		hProgram = loadShader(vss, camera_fss);error = GLES31.glGetError();
		displayTextureProgram = loadShader(vss, texture_fss);error = GLES31.glGetError();

		SurfaceTexture mSTexture = new SurfaceTexture (hTex[0]);error = GLES31.glGetError();
		return mSTexture;
	}

	/**
	 * Loads shaders.
	 *
	 * @param vertex_shader the vertex shader
	 * @param fragment_shader the fragment shader
	 * @return the program ID
	 */
	private static int loadShader ( String vertex_shader, String fragment_shader ) {
		int vshader = GLES31.glCreateShader(GLES31.GL_VERTEX_SHADER);
		GLES31.glShaderSource(vshader, vertex_shader);
		GLES31.glCompileShader(vshader);
		int[] compiled = new int[1];
		GLES31.glGetShaderiv(vshader, GLES31.GL_COMPILE_STATUS, compiled, 0);
		if (compiled[0] == 0) {
			Log.e("Shader", "Could not compile vshader");
			Log.v("Shader", "Could not compile vshader:"+GLES31.glGetShaderInfoLog(vshader));
			GLES31.glDeleteShader(vshader);
			vshader = 0;
		}

		int fshader = GLES31.glCreateShader(GLES31.GL_FRAGMENT_SHADER);
		GLES31.glShaderSource(fshader, fragment_shader);
		GLES31.glCompileShader(fshader);
		GLES31.glGetShaderiv(fshader, GLES31.GL_COMPILE_STATUS, compiled, 0);
		if (compiled[0] == 0) {
			Log.e("Shader", "Could not compile fshader");
			Log.v("Shader", "Could not compile fshader:"+GLES31.glGetShaderInfoLog(fshader));
			GLES31.glDeleteShader(fshader);
			fshader = 0;
		}

		int program = GLES31.glCreateProgram();
		GLES31.glAttachShader(program, vshader);
		GLES31.glAttachShader(program, fshader);
		GLES31.glLinkProgram(program);

		return program;
	}

	/**
	 * Deletes textures.
	 */
	public void deleteTex() {
		GLES31.glDeleteTextures (1, hTex, 0);
	}
}
