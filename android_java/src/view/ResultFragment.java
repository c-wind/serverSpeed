package view;

import jni.VPNJni;

import org.codecn.speed.R;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class ResultFragment extends Fragment {

	private static TextView textResultDeail;
	
	@Override
	public void setUserVisibleHint(boolean isVisibleToUser) {
		super.setUserVisibleHint(isVisibleToUser);
		if (isVisibleToUser) {
			textResultDeail.setText("");
			for (String msg : VPNJni.msgList) {
				textResultDeail.append(msg);
			}
		} else {
			// 相当于Fragment的onPause
		}
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fragment_result, container, false);
		textResultDeail = (TextView) rootView.findViewById(R.id.textViewResultDeatil);
		return rootView;
	}
}
