package net.sourceforge.gigalomania;

import org.libsdl.app.SDLActivity; 

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

public class Gigalomania extends SDLActivity {
    private static final String TAG = "Gigalomania";

    public static native void AndroidGrantedStoragePermission();

    private PermissionHandler permissionHandler;

    // JNI methods - note that these won't be called on the UI thread

	public void launchUrl(String url) {
        Log.d(TAG, "launchUrl(): " + url);
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    	startActivity(browserIntent);
	}

	public void showToast(final String toast) {
        Log.d(TAG, "showToast(): " + toast);
	    this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(Gigalomania.this, toast, Toast.LENGTH_LONG).show();
            }
        });
	}

	public void requestStoragePermission() {
        Log.d(TAG, "requestStoragePermission()");
	    this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
        	    permissionHandler.requestStoragePermission();
            }
        });
    }

    // end JNI methods

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        permissionHandler = new PermissionHandler(this);
        
        // keep screen active - see http://stackoverflow.com/questions/2131948/force-screen-on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

	@Override
	public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult: requestCode " + requestCode);
		permissionHandler.onRequestPermissionsResult(requestCode, grantResults);
	}
}
