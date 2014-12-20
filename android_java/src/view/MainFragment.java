package view;

import jni.VPNJni;

import org.codecn.speed.R;

import pthread.DataManager;
import pthread.VPNManager;
import android.app.Fragment;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

public class MainFragment extends Fragment {
	private EditText serverA;
	private EditText serverB;
	private EditText speed;
	private EditText packLen;
	private EditText timeout;
	private EditText max_time;
	private CheckBox checkA;
	private CheckBox checkB;
	private TextView resultTextView;
	private TextView detailTextView;
	private static Button btn;
	private static Button btnUpload;
	private RadioGroup rg;
	private int selectType = VPNJni.CHECK_TYPE_UDP_ECHO;
	private int detailLines = 0;
	public Handler handle;

	public void cleanResult() {
		resultTextView.setText("");
	}

	public void showResult(String msg) {
		resultTextView.append(msg);
	}

	public void showDetail(String msg) {
		if (detailLines++ > 10) {
			String body = detailTextView.getText().toString();
			detailTextView.setText(body.replaceFirst("(.*)\n?", ""));
			detailLines = 11;
		}
		detailTextView.append(msg);
	}

	private RadioGroup.OnCheckedChangeListener mChangeRadio = new RadioGroup.OnCheckedChangeListener() {
		@Override
		public void onCheckedChanged(RadioGroup group, int checkedId) {
			switch (checkedId) {
			case R.id.radioTcpConn:
				selectType = VPNJni.CHECK_TYPE_TCP_CONN;
				break;

			case R.id.radioTcpEcho:
				selectType = VPNJni.CHECK_TYPE_TCP_ECHO;
				break;

			case R.id.radioUdpEcho:
				selectType = VPNJni.CHECK_TYPE_UDP_ECHO;
				break;
			}
		}
	};

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fragment_main, container, false);
		btn = (Button) rootView.findViewById(R.id.buttonStart);
		btnUpload = (Button) rootView.findViewById(R.id.buttonUpload);
		serverA = (EditText) rootView.findViewById(R.id.editTextServerA);
		serverB = (EditText) rootView.findViewById(R.id.editTextServerB);
		speed = (EditText) rootView.findViewById(R.id.editTextSpeed);
		packLen = (EditText) rootView.findViewById(R.id.editTextPackLen);
		timeout = (EditText) rootView.findViewById(R.id.editTextTimeout);
		max_time = (EditText) rootView.findViewById(R.id.EditTextTimer);
		checkA = (CheckBox) rootView.findViewById(R.id.checkBoxA);
		checkB = (CheckBox) rootView.findViewById(R.id.checkBoxB);

		rg = (RadioGroup) rootView.findViewById(R.id.radioGroup);
		detailTextView = (TextView) rootView.findViewById(R.id.textViewDetailBody);
		resultTextView = (TextView) rootView.findViewById(R.id.textViewResultBody);
		rg.setOnCheckedChangeListener(mChangeRadio);

		this.handle = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				String rMsg = (String) msg.obj;
				switch (msg.what) {
				case VPNJni.ACTION_RESULT:
					showResult(rMsg);
					break;
				case VPNJni.ACTION_DETAIL:
					showDetail(rMsg);
					break;
				case VPNJni.ACTION_CHECK_OVER:
					btn.setEnabled(true);
					btn.setText("开始测速");
					break;
				case VPNJni.ACTION_UPLOAD_SUCC:
					btnUpload.setEnabled(true);
					btnUpload.setText(R.string.buttonUpload);
					showResult("上传结果成功\n");
					break;
				case VPNJni.ACTION_UPLOAD_FAIL:
					btnUpload.setEnabled(true);
					btnUpload.setText(R.string.buttonUpload);
					showResult("上传结果失败了，，，\n");
					break;
				}

				super.handleMessage(msg);
			}
		};

		VPNJni.handle = this.handle;

		btnUpload.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				DataManager.getInstance().uploadResult(handle);
				btnUpload.setText("上传中...");
				btnUpload.setEnabled(false);
			}
		});

		btn.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				String addrA = "";
				String addrB = "";
				int iTimes = 20;
				int iPackLen = 64;
				int iTimeout = 5;
				int iMaxTime = 60;

				if (checkA.isChecked()) {
					if (serverA.getText().toString().length() > 5) {
						addrA = serverA.getText().toString();
					} else {
						addrA = "122.224.73.165";
					}
				}

				if (checkB.isChecked()) {
					if (serverB.getText().toString().length() > 5) {
						addrB = serverB.getText().toString();
					} else {
						
						addrB = "221.228.205.190";
					}
				}

				if (addrA.length() == 0 && addrB.length() == 0) {
					Toast.makeText(MainActivity.defaultContext, "至少要选择一个节点才能进行测速", Toast.LENGTH_SHORT).show();
					return;
				}

				if (packLen.getText().toString().length() > 0) {
					iPackLen = Integer.parseInt(packLen.getText().toString().trim());
				}

				if (iPackLen > 1500 || iPackLen < 64) {
					Toast.makeText(MainActivity.defaultContext, "包长允许范围64~1500(字节)", Toast.LENGTH_SHORT).show();
					return;
				}

				if (timeout.getText().toString().length() > 0) {
					iTimeout = Integer.parseInt(timeout.getText().toString().trim());
				}

				if (iTimeout < 1 &&  iTimeout > 60) {
					Toast.makeText(MainActivity.defaultContext, "超时允许范围1~60(秒)", Toast.LENGTH_SHORT).show();
					return;
				}

				if (speed.getText().toString().length() > 0) {
					iTimes = Integer.parseInt(speed.getText().toString().trim());
				}

				if (iTimes > 20 && iTimes < 1) {
					Toast.makeText(MainActivity.defaultContext, "速率允许范围1~20次(每秒/次)", Toast.LENGTH_SHORT).show();
					return;
				}

				if (max_time.getText().toString().length() > 0) {
					iMaxTime = Integer.parseInt(max_time.getText().toString().trim());
				}

				if (iMaxTime > 300 && iMaxTime < 5) {
					Toast.makeText(MainActivity.defaultContext, "测速时长允许范围5~300次(秒)", Toast.LENGTH_SHORT).show();
					return;
				}

				VPNManager.getInstance().StartCheck(addrA, addrB, selectType, iPackLen, iTimes, iTimeout, iMaxTime);
				
				cleanResult();

				btn.setText("测速中...");
				btn.setEnabled(false);
			}
		});
		return rootView;
	}
}
