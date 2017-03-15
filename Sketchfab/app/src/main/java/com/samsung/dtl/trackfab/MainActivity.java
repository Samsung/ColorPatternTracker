package com.samsung.dtl.trackfab;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.koushikdutta.async.ByteBufferList;
import com.koushikdutta.async.DataEmitter;
import com.koushikdutta.async.callback.CompletedCallback;
import com.koushikdutta.async.callback.DataCallback;
import com.koushikdutta.async.http.WebSocket;
import com.koushikdutta.async.http.server.AsyncHttpServer;
import com.koushikdutta.async.http.server.AsyncHttpServerRequest;
import com.samsung.dtl.wearableremote.profile.BTReceiverClass;

public class MainActivity extends Activity
{
    private final Context context = this;
    private BTReceiverClass btReceiver;
    private Button button;

    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btReceiver = BTReceiverClass.newInstance(this);
        btReceiver.initBT(this);

        button = (Button)findViewById(R.id.buttonUrl);
        button.setOnClickListener(new OnClickListener()
            {
                @Override
                public void onClick(View arg0)
                {
                  Intent intent = new Intent(context, WebViewActivity.class);
                  startActivity(intent);
                }
            }
        );

        // First create a new server instance.
        AsyncHttpServer server = new AsyncHttpServer();
        // Add a websocket connection callback.
        server.websocket("/live", new socketCallbackClass());
        server.listen(4444);
    }

    public void onResume()
    {
        super.onResume();
        Intent intent = new Intent(context, WebViewActivity.class);
        startActivity(intent);
    }

    private class socketCallbackClass implements AsyncHttpServer.WebSocketRequestCallback
    {
        @Override
        public void onConnected(final WebSocket webSocket, AsyncHttpServerRequest request)
        {
            // Set up callbacks on the websocket instance.

            // Will get called when the client sends a string message.
            webSocket.setStringCallback(new WebSocket.StringCallback()
                {
                    @Override
                    public void onStringAvailable(String s)
                    {
                        webSocket.send("from server:" + s);
                        Log.e("TrackFab", "TrackFab got some string:" + s);
                    }
                }
            );

            // Will get called when the client sends a binary message.
            webSocket.setDataCallback(new DataCallback()
                {
                    @Override
                    public void onDataAvailable(DataEmitter emitter, ByteBufferList bb)
                    {
                        Log.e("TrackFab", "TrackFab got some data: " + bb.getAllByteArray().toString());
                        webSocket.send(bb.getAllByteArray());
                        bb.recycle();
                    }
                }
            );

            // Use this to clean up any references to your websocket
            // Will get called when the client closes.
            webSocket.setClosedCallback(new CompletedCallback()
                {
                    @Override
                    public void onCompleted(Exception ex)
                    {
                        Log.e("TrackFab", "TrackFab completed");
                    }
                }
            );

            // Start running the web socket.
            new runSocket().execute(webSocket);
        }
    }

    private class runSocket extends AsyncTask<WebSocket, Integer, Long>
    {
        protected Long doInBackground(WebSocket... webSocket)
        {
            int count = 0;
            if (null != webSocket[0])
            {
                while (true)
                {
                    webSocket[0].send(btReceiver.getX()+","+btReceiver.getY()+","+btReceiver.getZ());
                    count++;
                    try
                    {
                        Thread.sleep(17);
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
}