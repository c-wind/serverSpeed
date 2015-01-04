package view;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import jni.VPNJni;

import org.codecn.speed.R;

import pthread.DataManager;

import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import data.ReportInfo;

public class ReportAdapter extends BaseAdapter {

	private List<ReportInfo> data = new ArrayList<ReportInfo>();
	private LayoutInflater inflater;
	private final static ReportAdapter me = new ReportAdapter();

	private ReportAdapter() {
	}
	
	public static ReportAdapter GetInstance(LayoutInflater inflater) {
		me.inflater = inflater;
		return me;
	}

	public void AddMessageList(List<ReportInfo> l) {
		data.addAll(l);
		this.notifyDataSetChanged();
	}

	public void SetMessageList(List<ReportInfo> ji) {
		this.data.clear();
		this.AddMessageList(ji);
	}

	@Override
	public View getView(int position, View view, ViewGroup parent) {
		view = me.inflater.inflate(R.layout.result_box, null);
		bindView(view, position);
		return view;
	}

	public static String loadFileToString(File f) {
		BufferedReader br = null;
		String ret = null;
		try {
			br = new BufferedReader(new FileReader(f));
			String line = null;
			StringBuffer sb = new StringBuffer((int) f.length());
			while ((line = br.readLine()) != null) {
				sb.append(line).append("\n");
			}
			ret = sb.toString();
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
		return ret;
	}

	static SimpleDateFormat df = new SimpleDateFormat("HH:mm:ss");

	private void bindView(View view, int position) {
		ReportInfo report = data.get(position);
		TextView tTime = (TextView) view.findViewById(R.id.textViewTime);
		TextView tSize = (TextView) view.findViewById(R.id.textViewSize);
		TextView tStat = (TextView) view.findViewById(R.id.TextViewStatus);
		final Button bUpload = (Button) view.findViewById(R.id.buttonUpload);
		final int idx  = position;
		Button bDelete = (Button) view.findViewById(R.id.buttonDelete);

		// tTime.setText(report.file);
		tTime.setText(df.format(new Date(Integer.valueOf(report.time) * 1000L)));
		tSize.setText(String.format("%d", report.size));
		tStat.setText(VPNJni.getTypeStringExt(report.type));

		bUpload.setTag(report);
		bDelete.setTag(report);

		bUpload.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				Button b = (Button) arg0;
				final ReportInfo report = (ReportInfo) b.getTag();
				String str = loadFileToString(new File(report.file));

				Handler handle = new Handler() {
					@Override
					public void handleMessage(Message msg) {
						String rMsg = (String) msg.obj;
						switch (msg.what) {
						case VPNJni.ACTION_UPLOAD_SUCC:
							MainActivity.ShowToast("上传成功\n " + report.file);
							me.data.remove(idx);
							me.notifyDataSetChanged();
							new File(report.file).renameTo(new File(report.file
									+ ".upload"));
							break;
						case VPNJni.ACTION_UPLOAD_FAIL:
							MainActivity.ShowToast("上传出错\n " + report.file);
							bUpload.setEnabled(true);
							bUpload.setText(R.string.buttonUpload);
							break;
						}

						super.handleMessage(msg);
					}
				};
				DataManager.getInstance().uploadResult(handle, str);
				b.setText("上传中...");
				b.setEnabled(false);
			}
		});

		bDelete.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				Button b = (Button) arg0;
				ReportInfo report = (ReportInfo) b.getTag();
				if (new File(report.file).delete()) {
					MainActivity.ShowToast("删除成功\n " + report.file);
					me.data.remove(idx);
					me.notifyDataSetChanged();
				} else {
					MainActivity.ShowToast("删除失败\n" + report.file);
				}
			}
		});
	}

	@Override
	public int getCount() {
		return data.size();
	}

	@Override
	public Object getItem(int position) {
		return data.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

}
