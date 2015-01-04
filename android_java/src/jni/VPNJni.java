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

	public static String sd_path = "";

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

	public static String getTypeString(int t) {
		String tp = "";
		int type = t & 3;
		boolean isQos = (t & 4) == 4;
		boolean isStrong = (t & 16) == 16;

		System.out.println("t:" + t + " isQos:" + isQos + " isStrong:"
				+ isStrong);

		if (isQos) {
			tp = "Qos";
		}

		if (isStrong) {
			tp += " 强";
		} else {
			tp += " 弱";
		}

		switch (type) {
		case CHECK_TYPE_UDP_ECHO:
			tp += " UDP";
			break;
		case CHECK_TYPE_TCP_CONN:
			tp += " Conn";
			break;
		case CHECK_TYPE_TCP_ECHO:
			tp += " Echo";
			break;
		}

		return tp;
	}

	public static String getTypeStringExt(int t) {
		String tp = "";
		int type = t & 3;
		boolean isQos = (t & 4) == 4;
		boolean isStrong = (t & 16) == 16;

		if (isQos) {
			tp = "使用Qos";
		}

		if (isStrong) {
			tp += " 信号强";
		} else {
			tp += " 信号弱";
		}

		switch (type) {
		case CHECK_TYPE_UDP_ECHO:
			tp += " UDP";
			break;
		case CHECK_TYPE_TCP_CONN:
			tp += " Conn";
			break;
		case CHECK_TYPE_TCP_ECHO:
			tp += " Echo";
			break;
		}

		return tp;
	}

	public static String getDestString(int t) {
		if (t == 1)
			return "节点A";
		return "节点B";
	}

	public static native int checkStart(byte[] addrA, byte[] addrB, int type,
			int packLen, int times, int timeout, int max_time);

	public static native void DumpMessage();

	public static void MessageLoop() {
		DumpMessage();
	}

	public static native int checkInit(byte[] path);

	public static void checkStop() {
		Message msg = new Message();
		msg.what = ACTION_CHECK_OVER;
		msg.obj = (Object) String.format("%s 测速完成\n", df.format(new Date()));
		handle.sendMessage(msg);
	}

	public static String GetResultTypeStr(int size) {
		String str = "";
		switch (size) {
		case -1:
			str = "超时";
			break;
		case -2:
			str = "网络异常";
			break;
		}

		return str;
	}

	public static String GetResultStr(int size) {
		String str = "";
		switch (size) {
		case -1:
			str = "超时";
			break;
		case -2:
			str = "网络异常";
			break;
		default:
			str = String.format("%d毫秒", size);
			break;
		}

		return str;
	}

	public static void checkDetail(int id, int tm, int binType, int tag,
			int result, int size) {
		Message msg = new Message();
		int type = binType & 3;
		msg.what = ACTION_DETAIL;
		System.out.println("result:" + result);
		if (type == 2) {
			msg.obj = (Object) String.format("%s %s %s %s\n",
					df.format(new Date(tm * 1000L)), getDestString(tag),
					getTypeString(type), GetResultStr(result));
		} else {
			msg.obj = (Object) String.format("%s %s %s %s %d字节\n",
					df.format(new Date(tm * 1000L)), getDestString(tag),
					getTypeString(type), GetResultStr(result), size);
		}
		handle.sendMessage(msg);
	}

	public static String GetPackSizeStr(int size) {
		if (size == -1) {
			return "随机长度";
		}
		return String.format("%s字节", size);
	}

	public static void checkResult(int tm, int binType, int tag, int delay,
			int send, int recv, int drop, int pack_len) {
		Message msg = new Message();
		msg.what = ACTION_RESULT;
		int type = binType;
		String line = "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ";

		if (type == CHECK_TYPE_TCP_ECHO) {
			msg.obj = (Object) String.format(
					"%s %s %s\n　　　发送:%d 接收:%d 中断连接:%d%% 延迟:%d\n%s\n",
					getDestString(tag), getTypeStringExt(type),
					GetPackSizeStr(pack_len), send, recv, drop, delay, line);
		} else {
			msg.obj = (Object) String.format(
					"%s %s %s\n　　　发送:%d 接收:%d 失败:%d%% 延迟:%d\n%s\n",
					getDestString(tag), getTypeStringExt(type),
					GetPackSizeStr(pack_len), send, recv, drop, delay, line);
		}
		handle.sendMessage(msg);
		System.out.println("send result msg");
	}

}
