package pthread;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import jni.VPNJni;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class DataManager extends Thread {
	private Handler handler = null;
	private static final DataManager instance = new DataManager();
	List<String> list = new ArrayList<>();
	private String userId;
	private Handler viewHandler = null;

	public static final int SAVE_DETAIL = 1;
	public static final int UPLOAD_RESULT = 2;

	class CheckParam {
		public int type;
		public int time;
	}

	private DataManager() {
		super("vpn looper");
	}
	
	public void SetUserId(String s) {
		this.userId = s;
	}

	public static DataManager getInstance() {
		return instance;
	}

	private void saveDetailArray(String cp) {
		list.add(cp);
	}

	
	private boolean httpGetResponse(String body) {
		try {
			String head = String.format("POST /uploadData?UserId=%s HTTP/1.1\r\nUser-Agent: speed\r\nHost: 59.151.27.113:9004\r\nContent-Length: %d\r\nAccept: */*\r\n\r\n%s", this.userId, body.length(), body);
			Socket socket = new Socket("59.151.27.113", 9004);
			socket.setSoTimeout(3000);
			OutputStream os = socket.getOutputStream();
			os.write(head.getBytes());
			BufferedReader br = new BufferedReader(new InputStreamReader(socket.getInputStream()));
			String res = br.readLine();
			br.close();
			socket.close();
			if (res.contains("HTTP/1.1 200 OK")) {
				return true;
			}
			
		} catch (UnknownHostException e) {
			e.printStackTrace();
			return false;
		} catch (IOException e) {
			e.printStackTrace();
			return false;
		}
		return false;
	}

	private void uploadDetailResult(String body) {
		Message msg = new Message();
		msg.what = VPNJni.ACTION_UPLOAD_FAIL;
		if (httpGetResponse(body)) {
			msg.what = VPNJni.ACTION_UPLOAD_SUCC;
		}
		viewHandler.sendMessage(msg);
		
	}

	public void saveDetail(String r) {
		Message msg = new Message();
		msg.what = SAVE_DETAIL;
		msg.obj = r;
		this.handler.sendMessage(msg);
	}

	public void uploadResult(Handler h, String body) {
		this.viewHandler = h;
		Message msg = new Message();
		msg.what = UPLOAD_RESULT;
		msg.obj = body;
		this.handler.sendMessage(msg);
	}

	private final Timer timer = new Timer();
	private TimerTask task;

	@Override
	public void run() {

		Looper.prepare();
		handler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				super.handleMessage(msg);
				switch (msg.what) {
				case SAVE_DETAIL:
					saveDetailArray((String) msg.obj);
					break;
				case UPLOAD_RESULT:
					uploadDetailResult((String) msg.obj);
					break;
				}
			}
		};

		Looper.loop();

	}

}
