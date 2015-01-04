package view;

import jni.VPNJni;

import org.codecn.speed.R;

import pthread.VPNManager;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
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
	private CheckBox checkIsQos;
	private CheckBox checkIsStrong;
	private TextView resultTextView;
	private TextView detailTextView;
	private static Button btn;
	private RadioGroup rg;
	private int selectType = VPNJni.CHECK_TYPE_UDP_ECHO;
	public Handler handle;
	
	public void cleanResult() {
		resultTextView.setText("");
	}

	public void showResult(String msg) {
		resultTextView.append(msg);
	}

	public void showDetail(String msg) {
		String body = detailTextView.getText().toString();
		String[] kv = body.split("\n");

		if (kv.length >= 10) {
			int lastIdx  = body.lastIndexOf('\n');
			lastIdx  = body.lastIndexOf('\n', lastIdx);
			body = body.substring(0, lastIdx);
		}
		detailTextView.setText(msg + body);
	}

	private RadioGroup.OnCheckedChangeListener mChangeRadio = new RadioGroup.OnCheckedChangeListener() {
		@Override
		public void onCheckedChanged(RadioGroup group, int checkedId) {
			switch (checkedId) {
			case R.id.radioTcpConn:
				selectType = VPNJni.CHECK_TYPE_TCP_CONN;
				speed.setHint("1(次/秒)");
				break;

			case R.id.radioTcpEcho:
				selectType = VPNJni.CHECK_TYPE_TCP_ECHO;
				speed.setHint("20(包/秒)");
				break;

			case R.id.radioUdpEcho:
				selectType = VPNJni.CHECK_TYPE_UDP_ECHO;
				speed.setHint("20(包/秒)");
				break;
			}
		}
	};

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fragment_speed, container, false);
		btn = (Button) rootView.findViewById(R.id.buttonStart);
		serverA = (EditText) rootView.findViewById(R.id.editTextServerA);
		serverB = (EditText) rootView.findViewById(R.id.editTextServerB);
		speed = (EditText) rootView.findViewById(R.id.editTextSpeed);
		packLen = (EditText) rootView.findViewById(R.id.editTextPackLen);
		timeout = (EditText) rootView.findViewById(R.id.editTextTimeout);
		max_time = (EditText) rootView.findViewById(R.id.EditTextTimer);
		checkA = (CheckBox) rootView.findViewById(R.id.checkBoxA);
		checkB = (CheckBox) rootView.findViewById(R.id.checkBoxB);

		checkIsQos = (CheckBox) rootView.findViewById(R.id.isQos);
		checkIsStrong = (CheckBox) rootView.findViewById(R.id.isStrong);

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
				}

				super.handleMessage(msg);
			}
		};

		VPNJni.handle = this.handle;

		btn.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				String addrA = "";
				String addrB = "";
				int iTimes = 20;
				int iPackLen = -1;
				int iTimeout = 5;
				int iMaxTime = 120;
				int typeBin = selectType;
				
				if (MainActivity.imm.isActive()) {
					MainActivity.imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
				}

				
				
				if (checkIsQos.isChecked()) {
					typeBin |= 4;
				}
				
				if (checkIsStrong.isChecked()) {
					typeBin |= 16;
				}
				
				if (checkA.isChecked()) {
					if (serverA.getText().toString().length() > 5) {
						addrA = serverA.getText().toString();
					} else {
						addrA = "221.228.82.107";
					}
				}

				if (checkB.isChecked()) {
					if (serverB.getText().toString().length() > 5) {
						addrB = serverB.getText().toString();
					} else {
						addrB = "221.228.82.108";
					}
				}

				if (addrA.length() == 0 && addrB.length() == 0) {
					Toast.makeText(MainActivity.defaultContext, "至少要选择一个节点才能进行测速", Toast.LENGTH_SHORT).show();
					return;
				}

				if (packLen.getText().toString().length() > 0) {
					iPackLen = Integer.parseInt(packLen.getText().toString().trim());
					if (iPackLen > 1500 || iPackLen < 64) {
						Toast.makeText(MainActivity.defaultContext, "包长允许范围64~1500(字节)", Toast.LENGTH_SHORT).show();
						System.out.println("len:" + iPackLen);
						return;
					}
				}


				if (timeout.getText().toString().length() > 0) {
					iTimeout = Integer.parseInt(timeout.getText().toString().trim());
				}

				if (iTimeout < 1 ||  iTimeout > 60) {
					Toast.makeText(MainActivity.defaultContext, "超时允许范围1~60(秒)", Toast.LENGTH_SHORT).show();
					return;
				}

				if(selectType == 2) {
					iTimes = 1;
				}

				if (speed.getText().toString().length() > 0) {
					iTimes = Integer.parseInt(speed.getText().toString().trim());
				}

				if (iTimes > 100 || iTimes < 1) {
					Toast.makeText(MainActivity.defaultContext, "速率允许范围1~100次(每秒/次)", Toast.LENGTH_SHORT).show();
					return;
				}
				
				

				if (max_time.getText().toString().length() > 0) {
					iMaxTime = Integer.parseInt(max_time.getText().toString().trim());
				}
				
				if (iMaxTime * iTimes > 60000 || iMaxTime < 5) {
					Toast.makeText(MainActivity.defaultContext, "总发送数量(测速时长*发包速率)需小于60000次", Toast.LENGTH_SHORT).show();
					return;
				}

				VPNManager.getInstance().StartCheck(addrA, addrB, typeBin, iPackLen, iTimes, iTimeout, iMaxTime);
				
				cleanResult();

				btn.setText("测速中...");
				btn.setEnabled(false);
			}
		});
		return rootView;
	}
}
