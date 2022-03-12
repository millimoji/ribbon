package net.millimo.android.ribbon;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
 
import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import org.json.JSONObject;
import org.json.JSONException;

public class MediatorJava
{
	SoftKeyboard m_context;
	long m_imeHandle = 0;
	Thread m_imeThread;
	boolean m_exitFlag = false;
	String m_candidateJson = null;

	MediatorJava(SoftKeyboard context)
	{
		m_context = context;

		m_imeThread = new Thread() {
			public void run()
			{
				WorkerThreadForIme();
			}
		};
		m_imeThread.start();
	}
	public void Finalize()
	{
		if (m_imeHandle != 0) {
			ImeQuitRequest(m_imeHandle);
		}
		m_exitFlag = true;
		try {
			m_imeThread.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

	public void postImeRequest(String request)
	{
		if (m_imeHandle != 0) {
			ImeRequest(m_imeHandle, request);
		}
	}

	void WorkerThreadForIme()
	{
		m_imeHandle = ImeCreate();

		if (ImeIsInitialLaunch(m_imeHandle, 0) != 0)
		{
			ImeDestroy(m_imeHandle);
			m_imeHandle = 0;

			deployBundleAssets();

			m_imeHandle = ImeCreate();
			// Dictionary deployed
			ImeIsInitialLaunch(m_imeHandle, 1);
		}

		while (!m_exitFlag)
		{
			ImeEvent(m_imeHandle);
			String compositionState = ImeGetCompositionState(m_imeHandle);
			if (compositionState != null) {
				String commitedText = "";
				String compositionText = "";
				int compositionCursor = 1;
				int passThroughKey = 0;
				try {
					JSONObject json = new JSONObject(compositionState);
					commitedText = json.getString("commit");
					compositionText = json.getJSONArray("composition").getJSONObject(0).getString("display");
					compositionCursor = json.getInt("caret");
					passThroughKey = json.getInt("through");
				} catch (JSONException e) {
					e.printStackTrace();
				}
				m_context.updateCompositionString(commitedText, compositionText, compositionCursor, passThroughKey);
			}

			m_candidateJson = ImeGetCandidateState(m_imeHandle);
			if (m_candidateJson != null) {
				m_context.invokeMediatorTriggerInJs();
			}

			String keyboardState = ImeGetKeyboardState(m_imeHandle);
			if (keyboardState != null) {
				try {
					JSONObject json = new JSONObject(keyboardState);
					String nextKbdState = json.getString("keyboard");
					if (nextKbdState.equals("quit")) {
						m_context.quitKeyboard();
					}
				} catch (JSONException e) {
					e.printStackTrace();
				}
			}
		}

		ImeDestroy(m_imeHandle);
		m_imeHandle = 0;
	}

	// cpp interface
	private native int ImeIsInitialLaunch(long imeHandle, int doUpdate);
	private native void ImeRequest(long imeHandle, String arguments);
	private native void ImeQuitRequest(long imeHandle);
	private native long ImeCreate();
	private native void ImeEvent(long imeHandle);
	private native String ImeGetCompositionState(long imeHandle);
	private native String ImeGetCandidateState(long imeHandle);
	private native String ImeGetKeyboardState(long imeHandle);
	private native void ImeDestroy(long imeHandle);

	@JavascriptInterface
	public void NativeRequest(String request) {
		if (m_imeHandle != 0) {
			ImeRequest(m_imeHandle, request);
		}
	}
	@JavascriptInterface
	public String GetCandidateState() {
		if (m_imeHandle != 0) {
			return m_candidateJson;
		}
		return null;
	}

    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.hellojni/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("imecore");
    }

	// assets
	private static final String DATA_DIR_NAME = "data";
	private static final boolean USE_EXTERNAL_STORAGE = false; //; //use inner storage.
	private StorageManager m_storageManager;

	private void deployBundleAssets()
	{
        try {
			m_storageManager = new StorageManager(m_context, DATA_DIR_NAME, USE_EXTERNAL_STORAGE);
        	m_storageManager.initData();
        } catch (Throwable th) {
        	th.printStackTrace();
        }
	}
}
