package pthread;

import java.util.Timer;
import java.util.TimerTask;

import jni.VPNJni;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class VPNManager extends Thread {
	private Handler handler = null;
	private static final VPNManager instance = new VPNManager();

	public static final int START_CHECK = 1;
	public static final int TIMER_CHECK = 2;

	class CheckParam {
		public String normal;
		public String qos;
		public int type;
		public int times;
		public int max_time;
		public int packLen;
		public int timeout;
	}

	private VPNManager() {
		super("vpn looper");
	}

	public static VPNManager getInstance() {
		return instance;
	}

	private void startCheck(CheckParam cp) {
		VPNJni.checkStart(cp.normal.getBytes(), cp.qos.getBytes(), cp.type, cp.packLen, cp.times, cp.timeout, cp.max_time);
	}

	public  void StartCheck(String normal, String qos, int type, int packLen, int times, int timeout, int max_time) {
		CheckParam cp = new CheckParam();
		cp.normal = normal;
		cp.qos = qos;
		cp.type = type;
		cp.times = times;
		cp.max_time = max_time;
		cp.packLen = packLen;
		cp.timeout = timeout;

		Message msg = new Message();
		msg.what = START_CHECK;
		msg.obj = cp;
		this.handler.sendMessage(msg);
	}

	private final Timer timer = new Timer();
	private TimerTask task;

	@Override
	public void run() {
		VPNJni.checkInit();
		
		task = new TimerTask() {
			@Override
			public void run() {
				Message message = new Message();
				message.what = TIMER_CHECK;
				handler.sendMessage(message);
			}
		};
		timer.schedule(task, 10, 10);

		Looper.prepare();
		handler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				super.handleMessage(msg);
				switch (msg.what) {
				case START_CHECK:
					startCheck((CheckParam) msg.obj);
					break;
				case TIMER_CHECK:
					VPNJni.MessageLoop();
					break;
				}
			}
		};

		Looper.loop();

	}

}
