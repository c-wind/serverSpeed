package jni;


import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import pthread.DataManager;

import android.os.Handler;
import android.os.Message;

public class VPNJni {
	static {
		System.loadLibrary("speed");
	}

	public static List<String> msgList = new ArrayList<>();

	public static Handler handle;
	public final static int ACTION_DETAIL = 0;
	public final static int ACTION_RESULT = 1;
	public final static int ACTION_CHECK_OVER = 2;
	public final static int ACTION_UPLOAD_SUCC = 3;
	public final static int ACTION_UPLOAD_FAIL = 4;

	public final static int CHECK_TYPE_UDP_ECHO = 1;
	public final static int CHECK_TYPE_TCP_CONN = 2;
	public final static int CHECK_TYPE_TCP_ECHO = 3;

	public final static int CHECK_DEST_NORMAL = 0;
	public final static int CHECK_DEST_QOS = 1;

	static SimpleDateFormat df = new SimpleDateFormat("HH:mm:ss");

	static String getTypeString(int t) {
		switch (t) {
		case CHECK_TYPE_UDP_ECHO:
			return "UDP";
		case CHECK_TYPE_TCP_CONN:
			return "Conn";
		case CHECK_TYPE_TCP_ECHO:
			return "Echo";
		}
		return "UnDef";
	}

	static String getDestString(int t) {
		if (t == 1)
			return "节点A";
		return "节点B";
	}

	public static native int checkStart(byte[] srcAddr, byte[] qosAddr, int type, int packLen, int times, int timeout,
			int max_time);

	public static native void DumpMessage();

	public static void MessageLoop() {
		DumpMessage();
	}

	public static native void checkInit();

	public static void checkStop() {
		Message msg = new Message();
		msg.what = ACTION_CHECK_OVER;
		msg.obj = (Object) String.format("%s 测速完成\n", df.format(new Date()));
		handle.sendMessage(msg);
	}

	public static void checkDetail(int id, int tm, int type, int tag, int result) {
		String upStr = String.format("%d %d %s %s %d\n", id, tm, getDestString(tag), getTypeString(type), result);
		DataManager.getInstance().saveDetail(upStr);
		if (msgList.size() > 30) {
			msgList.remove(0);
		}
		msgList.add(String.format("%s %s %s %d\n", df.format(new Date(tm * 1000L)), getDestString(tag),
				getTypeString(type), result));

		Message msg = new Message();
		msg.what = ACTION_DETAIL;
		msg.obj = (Object) String.format("%s %s %s %d\n", df.format(new Date(tm * 1000L)), getDestString(tag),
				getTypeString(type), result);
		handle.sendMessage(msg);
	}

	public static void checkResult(int tm, int type, int tag, int drop, int delay) {
		Message msg = new Message();
		msg.what = ACTION_RESULT;
		if (type == CHECK_TYPE_TCP_ECHO) {
			msg.obj = (Object) String.format("%s %s 延迟:%d\n", getDestString(tag), getTypeString(type), delay);
		} else {
			msg.obj = (Object) String.format("%s %s 失败:%d%% 延迟:%d\n", getDestString(tag), getTypeString(type), drop,
					delay);
		}
		handle.sendMessage(msg);
		System.out.println("send result msg");
	}

}
