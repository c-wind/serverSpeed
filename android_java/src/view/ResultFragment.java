package view;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import jni.VPNJni;

import org.codecn.speed.R;

import android.app.Fragment;
import android.os.Bundle;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class ResultFragment extends Fragment {

	private static TextView textResultDeail;
	static SimpleDateFormat df = new SimpleDateFormat("yy-MM-dd HH:mm:ss");

	private String parseResultLine(String line) {
		String str = "";
		//1419387109 0 17 1 0 49
		String[] kv = line.split(" ");
		if (kv.length != 9)
			return str;
		
		str = String.format("%s %s\n%s\t\t数据包:%s\n失败率:%s%%\t\t发送:%s\t\t接收:%s\n平均延迟:%s\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n", 
				df.format(new Date(Integer.parseInt(kv[0]) * 1000L)), 
				VPNJni.getTypeStringExt(Integer.parseInt(kv[2])), 
				VPNJni.getDestString(Integer.parseInt(kv[3])),
				VPNJni.GetPackSizeStr(Integer.parseInt(kv[8])),
				kv[7],
				kv[5], 
				kv[6],
				kv[4]);
		return str;
	}

	private List<String> getDataList(String file_name) {
		File f = new File(file_name);
		List<String> list = new ArrayList<>();
		BufferedReader br = null;
		try {
			br = new BufferedReader(new FileReader(f));
			String line = null;
			while ((line = br.readLine()) != null) {
				list.add(parseResultLine(line));
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (br != null) {
				try {
					br.close();
				} catch (Exception e) {
				}
			}
		}
		return list;
	}

	@Override
	public void setUserVisibleHint(boolean isVisibleToUser) {
		super.setUserVisibleHint(isVisibleToUser);
		File f = Environment.getExternalStorageDirectory();
		String file_name = String.format("%s/server_speed/result", f.getPath());
		if (isVisibleToUser) {
			textResultDeail.setText("");
			for (String msg : getDataList(file_name)) {
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
