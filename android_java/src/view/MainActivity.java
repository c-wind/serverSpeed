package view;

import org.codecn.speed.R;
import pthread.DataManager;
import pthread.VPNManager;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.os.Bundle;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.telephony.TelephonyManager;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

public class MainActivity extends Activity {
	SectionsPagerAdapter mSectionsPagerAdapter;
	ViewPager mViewPager;
	public static Context defaultContext = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
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
		System.out.println("ÍË³öÁË"); 
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
			if (position == 0) {
				return new  MainFragment();
			}
			return new ResultFragment();
		}

		@Override
		public int getCount() {
			return 2;
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


}
