package com.samsung.dtl.trackfab;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;

public class WebViewActivity extends Activity
{
    private WebView webView;

    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.webview);

        webView = (WebView)findViewById(R.id.webView);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.loadUrl("file:///android_res/raw/index.html");

        // TODO: LATER: make background task updating position run in the WebViewActivity without websocket.
        //new MainActivity.runSocket().execute(webView);
    }

    /*
    TODO: LATER: make background task updating position run in the WebViewActivity without websocket.

    private class runSocket extends AsyncTask<WebSocket, Integer, Long>
    {
        protected Long doInBackground(WebSocket... webSocket)
        {
            int count = 0;
            if (null != webSocket[0])
            {
                while (true)
                {
                    ByteBufferList bbl;
                    ByteBuffer bb = ByteBuffer.allocate(12);
                    bb.putFloat(0, btReceiver.getX());
                    bb.putFloat(1, btReceiver.getY());
                    bb.putFloat(2, btReceiver.getZ());

                    webView.evaluateJavascript("changeView(x,y,z)");


                    count++;
                    try
                    {
                        Thread.sleep(250);
                    }
                    catch (InterruptedException e)
                    {
                        e.printStackTrace();
                    }
                }
            }

            return 0L;
        }
    }
    */
}