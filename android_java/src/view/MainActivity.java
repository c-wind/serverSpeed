package view;

import java.io.File;

import jni.VPNJni;

import org.codecn.speed.R;

import pthread.DataManager;
import pthread.VPNManager;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Environment;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.telephony.CellLocation;
import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.telephony.cdma.CdmaCellLocation;
import android.telephony.gsm.GsmCellLocation;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
	SectionsPagerAdapter mSectionsPagerAdapter;
	ViewPager mViewPager;
	public static Context defaultContext = null;
	private LayoutParams wmParams;
	private WindowManager mWindowManager;
	private String TAG = "SPEED";
	public static InputMethodManager imm = null;
	private LinearLayout mFloatLayout;
	public static TextView floatTextView;
	
	public static void ShowToast(String msg) {
		Toast.makeText(MainActivity.defaultContext, msg, Toast.LENGTH_SHORT).show();
	}

	private void createFloatView() {
		// ��ȡLayoutParams����
		wmParams = new WindowManager.LayoutParams();

		// ��ȡ����LocalWindowManager����
		// mWindowManager = this.getWindowManager();
		mWindowManager = getWindow().getWindowManager();

		// ��ȡ����CompatModeWrapper����
		// mWindowManager = (WindowManager)
		// getApplication().getSystemService(Context.WINDOW_SERVICE);
		wmParams.type = LayoutParams.TYPE_PHONE;
		wmParams.format = PixelFormat.RGBA_8888;
		wmParams.flags = LayoutParams.FLAG_NOT_FOCUSABLE;
		wmParams.gravity = Gravity.LEFT | Gravity.TOP;
		wmParams.x = 0;
		wmParams.y = 0;
		wmParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
		wmParams.height = WindowManager.LayoutParams.WRAP_CONTENT;

		LayoutInflater inflater = this.getLayoutInflater();// LayoutInflater.from(getApplication());

		mFloatLayout = (LinearLayout) inflater.inflate(R.layout.float_layout, null);
		mWindowManager.addView(mFloatLayout, wmParams);
		// setContentView(R.layout.main);
		floatTextView = (TextView) mFloatLayout.findViewById(R.id.textViewFloat);

		// �󶨴����ƶ�����
		floatTextView.setOnTouchListener(new OnTouchListener() {

			@Override
			public boolean onTouch(View v, MotionEvent event) {
				// TODO Auto-generated method stub
				wmParams.x = (int) event.getRawX() - mFloatLayout.getWidth() / 2;
				// 25Ϊ״̬���߶�
				wmParams.y = (int) event.getRawY() - mFloatLayout.getHeight() / 2 - 40;
				mWindowManager.updateViewLayout(mFloatLayout, wmParams);
				return false;
			}
		});

		// �󶨵������
		floatTextView.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {

			}
		});

	}

	private void initDevice() {
		TelephonyManager tel = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
		tel.listen(new TelLocationListener(), PhoneStateListener.LISTEN_CELL_LOCATION);
		tel.listen(new TelLocationListener(), PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);

	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.createFloatView();
		this.initDevice();
		imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		setContentView(R.layout.activity_main);
		mSectionsPagerAdapter = new SectionsPagerAdapter(getFragmentManager());
		mViewPager = (ViewPager) findViewById(R.id.pager);
		mViewPager.setAdapter(mSectionsPagerAdapter);
		defaultContext = getApplicationContext();
		TelephonyManager mTelephonyMgr = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
		String imsi = mTelephonyMgr.getSubscriberId();

		VPNManager vpn = VPNManager.getInstance();
		new Thread(vpn).start();

		DataManager dm = DataManager.getInstance();
		dm.SetUserId(imsi);
		new Thread(dm).start();

	}

	@Override
	public void onBackPressed() {
		super.onBackPressed();
		System.out.println("�˳���");
		System.exit(0);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int id = item.getItemId();
		if (id == R.id.action_exit) {
			this.finish();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	public class SectionsPagerAdapter extends FragmentPagerAdapter {
		public SectionsPagerAdapter(FragmentManager fm) {
			super(fm);
		}

		@Override
		public Fragment getItem(int position) {
			switch (position) {
			case 1:
				return new ResultFragment();
			case 2:
				return new ReportFragment();
			}
			return new MainFragment();
		}

		@Override
		public int getCount() {
			return 3;
		}

		@Override
		public CharSequence getPageTitle(int position) {
			switch (position) {
			case 0:
				return "title0";
			case 1:
				return "title1";
			case 2:
				return "title2";
			}
			return null;
		}
	}

	public static String signalStr = "";
	public static String baseIdStr = "";

	// MCC �� ���ʱ�ʶ Mobile Country Code �й�Ϊ460
	// MNC �� ��Ӫ�̱�־ Mobile Network Code �й��ƶ�Ϊ00���й���ͨΪ01,�й�����Ϊ03
	// CID �� ��վ��ʶ cell id
	// LOC �� �����ʶ location id
	public class TelLocationListener extends PhoneStateListener {

		@Override
		public void onSignalStrengthsChanged(SignalStrength signalStrength) {
			super.onSignalStrengthsChanged(signalStrength);
			signalStr = String.format("%d", signalStrength.getEvdoDbm());
			MainActivity.floatTextView.setText(String.format("��վ:%s\n�ź�:%s", baseIdStr, signalStr));
		}

		public void onCellLocationChanged(CellLocation location) {
			super.onCellLocationChanged(location);

			if (location == null)
				return;

			int mcc = -1;
			int mnc = -1;

			TelephonyManager tel = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
			String Operator = tel.getNetworkOperator();
			if (Operator != null && Operator.length() >= 5) {
				mcc = Integer.valueOf(Operator.substring(0, 3));
				mnc = Integer.valueOf(Operator.substring(3, 5));
			}

			if (location.getClass().equals(GsmCellLocation.class)) {
				GsmCellLocation loc = (GsmCellLocation) location;
				baseIdStr = String.format("%d", loc.getCid());
				MainActivity.floatTextView.setText(String.format("��վ:%s\n�ź�:%s", baseIdStr, signalStr));
			} else if (location.getClass().equals(CdmaCellLocation.class)) {
				CdmaCellLocation loc = (CdmaCellLocation) location;
				baseIdStr = String.format("%d", loc.getBaseStationId());
				MainActivity.floatTextView.setText(String.format("��վ:%s\n�ź�:%s", baseIdStr, signalStr));
			}
		}
	}
}
