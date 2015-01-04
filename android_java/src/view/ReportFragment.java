package view;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;

import org.codecn.speed.R;

import android.app.Fragment;
import android.os.Bundle;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import data.ReportInfo;

public class ReportFragment extends Fragment {

	private static ListView listViewReport;
	private static ReportAdapter adp = null;

	private int readStatusFromFile(File f) {
		int r = 0;
		byte[] buf = new byte[16];

		FileInputStream fi;
		try {
			fi = new FileInputStream(f);
			int n = fi.read(buf);
			for (int i = 0; i < n; i++) {
				if (buf[i] == ' ' || buf[i] == '\n') {
					n = i;
					break;
				}
			}
			String str = new String(buf, "UTF-8");
			r = Integer.valueOf(str.substring(0, n));
			fi.close();
		} catch (Exception e) {
			e.printStackTrace();
			return r;
		}
		return r;
	}
	
	private ReportInfo getReportInfoFromFile(File f) {
		ReportInfo r = new ReportInfo();
		r.file = f.getPath();
		r.time = f.getName();
		r.type = readStatusFromFile(f);
		r.size = f.length();

		return r;
	}

	private List<ReportInfo> getReportList() {
		List<ReportInfo> list = new ArrayList<ReportInfo>();
		File ef = Environment.getExternalStorageDirectory();
		File[] files = new File(ef.getPath() + "/server_speed/").listFiles();
		for (int i = 0; i < files.length; i++) {
			File f = files[i];
			if (f.isFile()) {
				System.out.println(f.getPath());
				if (f.getPath().endsWith("result"))
					continue;
				if (f.getPath().endsWith("log"))
					continue;
				if (f.getPath().endsWith(".tmp"))
					continue;
				if (f.getPath().endsWith(".upload"))
					continue;
				ReportInfo r = getReportInfoFromFile(f);
				list.add(r);
			}
		}

		return list;
	}

	@Override
	public void setUserVisibleHint(boolean isVisibleToUser) {
		super.setUserVisibleHint(isVisibleToUser);
		if (isVisibleToUser) {
			List<ReportInfo> list = getReportList();
			adp.SetMessageList(list);
		} else {
			// 相当于Fragment的onPause
		}
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fragment_report, container, false);
		listViewReport = (ListView) rootView.findViewById(R.id.listViewReport);
		adp = ReportAdapter.GetInstance(inflater);
		listViewReport.setAdapter(adp);
		return rootView;
	}
}
