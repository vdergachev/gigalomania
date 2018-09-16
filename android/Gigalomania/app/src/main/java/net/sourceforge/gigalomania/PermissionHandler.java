package net.sourceforge.gigalomania;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;

/**	Android 6+ permission handling:
 */
public class PermissionHandler {
	private static final String TAG = "PermissionHandler";

	private final Activity activity;

	final private int MY_PERMISSIONS_REQUEST_STORAGE = 0;

	PermissionHandler(Activity activity) {
	    this.activity = activity;
    }

	/** Show a "rationale" to the user for needing a particular permission, then request that permission again
	 *  once they close the dialog.
	 */
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
	private void showRequestPermissionRationale(final int permission_code) {
		Log.d(TAG, "showRequestPermissionRational: " + permission_code);
		if( Build.VERSION.SDK_INT < Build.VERSION_CODES.M ) {
			Log.e(TAG, "shouldn't be requesting permissions for pre-Android M!");
			return;
		}

		boolean ok = true;
		String [] permissions = null;
		String message = "";
		switch (permission_code) {
			case MY_PERMISSIONS_REQUEST_STORAGE:
				Log.d(TAG, "display rationale for storage permission");
				permissions = new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE};
				message = "Storage read/write permission is required for save games";
				break;
			default:
				Log.e(TAG, "showRequestPermissionRational unknown permission_code: " + permission_code);
				ok = false;
				break;
		}

		if( ok ) {
			final String [] permissions_f = permissions;
			new AlertDialog.Builder(activity)
			.setTitle("Permission required")
			.setMessage(message)
			.setIcon(android.R.drawable.ic_dialog_alert)
        	.setPositiveButton(android.R.string.ok, null)
			.setOnDismissListener(new DialogInterface.OnDismissListener() {
				public void onDismiss(DialogInterface dialog) {
					Log.d(TAG, "requesting permission...");
					ActivityCompat.requestPermissions(activity, permissions_f, permission_code);
				}
			}).show();
		}
	}

	void requestStoragePermission() {
		Log.d(TAG, "requestStoragePermission");
		if( Build.VERSION.SDK_INT < Build.VERSION_CODES.M ) {
			Log.d(TAG, "shouldn't need to be requesting permissions for pre-Android M");
			return;
		}
		if( ContextCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED ) {
			Log.d(TAG, "already has permission");
			return;
		}

		if( ActivityCompat.shouldShowRequestPermissionRationale(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE) ) {
	        // Show an explanation to the user *asynchronously* -- don't block
	        // this thread waiting for the user's response! After the user
	        // sees the explanation, try again to request the permission.
	    	showRequestPermissionRationale(MY_PERMISSIONS_REQUEST_STORAGE);
	    }
	    else {
	    	// Can go ahead and request the permission
			Log.d(TAG, "requesting storage permission...");
	        ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, MY_PERMISSIONS_REQUEST_STORAGE);
        }
    }

	public void onRequestPermissionsResult(int requestCode, @NonNull int[] grantResults) {
		Log.d(TAG, "onRequestPermissionsResult: requestCode " + requestCode);
		if( Build.VERSION.SDK_INT < Build.VERSION_CODES.M ) {
			Log.e(TAG, "shouldn't be requesting permissions for pre-Android M!");
			return;
		}

		switch( requestCode ) {
	        case MY_PERMISSIONS_REQUEST_STORAGE:
	        {
	            // If request is cancelled, the result arrays are empty.
	            if( grantResults.length > 0
	                && grantResults[0] == PackageManager.PERMISSION_GRANTED ) {
	                // permission was granted, yay! Do the
	                // contacts-related task you need to do.
					Log.d(TAG, "storage permission granted");
					Gigalomania.AndroidGrantedStoragePermission();
	            }
	            else {
					Log.d(TAG, "storage permission denied");
	                // permission denied, boo! Disable the
	                // functionality that depends on this permission.
	            }
	            return;
	        }
	        default:
	        {
				Log.e(TAG, "unknown requestCode " + requestCode);
	        }
	    }
	}
}
