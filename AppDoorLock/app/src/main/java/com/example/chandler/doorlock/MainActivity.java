package com.example.chandler.doorlock;

import android.content.DialogInterface;
import android.net.DhcpInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.support.v7.app.AlertDialog;
import android.view.View;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.ListView;

import android.content.Context;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;

import android.os.Handler;
import android.os.Message;
import android.graphics.Color;
import android.view.Gravity;
import android.widget.Toast;
import org.json.JSONObject;
import org.json.JSONException;

import java.net.Socket;

import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

public class MainActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener {
    private TextView textHello;
    private Button btnSearch;
    private Button btnPassword;
    private Button btnDev;
    private ListView listView;
    private EditText ssid_wifi;
    private EditText key_wifi;
    private Button set_wifi;
    private int iNumberPassword = 0;
    private int iPort = 0;
    private int iIndex = 1;
    private String stringAddress;
    Handler handlerSocketRev = null;
    mAdapter madapter;
    private int iPasswordId = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        textHello = (TextView)findViewById(R.id.hello);
        ssid_wifi = (EditText)findViewById(R.id.ssid_wifi) ;
        key_wifi = (EditText)findViewById(R.id.key_wifi);
        set_wifi = (Button)findViewById(R.id.set_wifi);
        btnSearch = (Button)findViewById(R.id.search_btn);
        btnPassword = (Button)findViewById(R.id.password_btn);
        btnDev = (Button)findViewById(R.id.dev_btn);
        listView = (ListView) findViewById(R.id.list_view);
        List<Data> mData = new LinkedList<Data>();
        mData.add(new Data(iIndex++, "暂无数据"));
        for (int i = 0; i < mData.size(); i++){
            Log.i("PCT", mData.get(i).getIndex()+ mData.get(i).getContent());
        }
        madapter = new mAdapter((LinkedList<Data>) mData, MainActivity.this);
        listView.setAdapter(madapter);

        btnSearch.setVisibility(View.VISIBLE);
        btnPassword.setVisibility(View.INVISIBLE);
        btnDev.setVisibility(View.INVISIBLE);
        textHello.setVisibility(View.INVISIBLE);
        ssid_wifi.setVisibility(View.INVISIBLE);
        key_wifi.setVisibility(View.INVISIBLE);
        set_wifi.setVisibility(View.INVISIBLE);
        textHello.setText("欢迎使用拓邦智能门锁演示系统");

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.setDrawerListener(toggle);
        toggle.syncState();

        NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);

        handlerSocketRev = new Handler()
        {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                switch (msg.what){
                    case (0x01):{//搜索到主机
                        textHello.setText(msg.obj.toString());
                        madapter.add(new Data(iIndex++, msg.obj.toString()));
                        mToast("操作成功",Toast.LENGTH_SHORT, getApplicationContext() );

                    }break;
                    case (0x02):{
                        textHello.setText(msg.obj.toString());
                        mToast("操作失败"+msg.obj.toString(),Toast.LENGTH_SHORT, getApplicationContext() );
                    }break;
                    case (0x0f):{
                        textHello.setText(msg.obj.toString());
                        madapter.add(new Data(iIndex++, msg.obj.toString()));
                        mToast("主机搜索成功",Toast.LENGTH_SHORT, getApplicationContext());

                        textHello.setText("获取上报事件");
                        btnSearch.setVisibility(View.INVISIBLE);
                        btnPassword.setVisibility(View.INVISIBLE);
                        btnDev.setVisibility(View.VISIBLE);

                        new SocketRecvThread().start();
                    }break;
                    case (0x0a):{
                        madapter.add(new Data(iIndex++, msg.obj.toString()));

                    }break;
                    default:
                        break;
                }
            }
        };
        btnSearch.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                textHello.setText("正在搜索主机...");
                mToast("searchIotcServer", Toast.LENGTH_SHORT, getApplicationContext());
                Log.i("PCT", "Start Search Thread");
                new SocketSearchThread().start();
            }
        });
        btnDev.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mToast("搜索门锁", Toast.LENGTH_SHORT, getApplicationContext());
                String stringSearchDoor = "{\"type\":22,\"sequence\":0}";
                new SocketSendThread(stringSearchDoor, 0x8000).start();
                new Handler().postDelayed(new Runnable(){
                    public void run() {
                        //execute the task
                    }
                }, 3000);
                Log.i("PCT", "Start Get Devices Lists");
                new DoorLockSearchThread().start();
            }
        });
    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            new AlertDialog.Builder(this)
            .setTitle("确认退出")
            .setIcon(android.R.drawable.ic_dialog_info)
            .setPositiveButton("返回", new DialogInterface.OnClickListener(){
                @Override
                public void onClick(DialogInterface dialog, int which){
                    mToast("back", Toast.LENGTH_LONG, getApplicationContext());
                }
            }).setNegativeButton("确定", new DialogInterface.OnClickListener(){
                @Override
                public void onClick(DialogInterface dialog, int which){
                    MainActivity.super.onBackPressed();
                }
            }).show();
            //super.onBackPressed();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @SuppressWarnings("StatementWithEmptyBody")
    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        // Handle navigation view item clicks here.
        int id = item.getItemId();

        if (id == R.id.nav_camera) {//搜索门锁
            // Handle the camera action
            listView.setVisibility(View.VISIBLE);
            btnDev.setVisibility(View.VISIBLE);
            btnPassword.setVisibility(View.INVISIBLE);
            ssid_wifi.setVisibility(View.INVISIBLE);
            key_wifi.setVisibility(View.INVISIBLE);
            set_wifi.setVisibility(View.INVISIBLE);
        } else if (id == R.id.nav_gallery) {//临时密码
            listView.setVisibility(View.VISIBLE);
            btnSearch.setVisibility(View.INVISIBLE);
            btnDev.setVisibility(View.INVISIBLE);
            ssid_wifi.setVisibility(View.INVISIBLE);
            key_wifi.setVisibility(View.INVISIBLE);
            set_wifi.setVisibility(View.INVISIBLE);
            btnPassword.setVisibility(View.VISIBLE);

            btnPassword.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    String stringPassword = "*";

                    HashSet intHash = new HashSet();
                    Random random = new Random();
                    int iNumber = 0;
                    while (iNumber < 6){
                        int iRandom = random.nextInt(10);
                        if(!intHash.contains(iRandom)){
                            intHash.add(iRandom);
                            stringPassword += iRandom;
                            iNumber++;
                        }
                    }
                    stringPassword+="#";
                    textHello.setText("临时密码为："+stringPassword);
                    madapter.add(new Data(iIndex++, "临时密码为："+stringPassword));
                    iPasswordId = iPasswordId +1;
                    if(iPasswordId==5){
                        iPasswordId = 0;
                    }
                    String stringAddPassword = "{\"type\":240,\"sequence\":0,\"mac\":0,\"id\":"+ Integer.toString(iPasswordId) +"," +
                            "\"available\":1,\"time_start\":0,\"time_end\":1510629973,\"length\":8," +
                            "\"password\":\""+stringPassword+"\"}";
                    new SocketSendThread(stringAddPassword, 0x8000).start();
                }
            });
        } else if (id == R.id.nav_slideshow) {//配置网络
            btnDev.setVisibility(View.INVISIBLE);
            textHello.setVisibility(View.INVISIBLE);
            btnSearch.setVisibility(View.INVISIBLE);
            btnPassword.setVisibility(View.INVISIBLE);
            listView.setVisibility(View.INVISIBLE);
            ssid_wifi.setVisibility(View.VISIBLE);
            key_wifi.setVisibility(View.VISIBLE);
            set_wifi.setVisibility(View.VISIBLE);

            WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            DhcpInfo di = wifiManager.getDhcpInfo();
            long getewayIpL=di.gateway;
            final String gatway_ip=long2ip(getewayIpL);//网关地址
            Log.i("PCT", wifiInfo.getBSSID());
            Log.i("PCT", gatway_ip);
            final String string_ssid = wifiInfo.getSSID();
            ssid_wifi.setText(wifiInfo.getSSID());

            set_wifi.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    String ssid_string = ssid_wifi.getText().toString();
                    String key_string = key_wifi.getText().toString();
                    Log.i("PCT", key_string);
                    String cmd = "{\"type\":17,\"ssid\":\""+ ssid_string +"\",\"key\":\"" + key_string + "\"}";
                    new WifiSettingThread(cmd, 0x8001, "192.168.1.1", 7787).start();
                }
            });
        } else if (id == R.id.nav_manage) {//搜索主机
            btnDev.setVisibility(View.INVISIBLE);
            textHello.setVisibility(View.INVISIBLE);
            btnSearch.setVisibility(View.VISIBLE);
            btnPassword.setVisibility(View.INVISIBLE);
            listView.setVisibility(View.VISIBLE);
            ssid_wifi.setVisibility(View.INVISIBLE);
            key_wifi.setVisibility(View.INVISIBLE);
            set_wifi.setVisibility(View.INVISIBLE);
        } else if (id == R.id.nav_share) {
        } else if (id == R.id.nav_send) {
        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }
    private class SocketSearchThread extends Thread{
        private boolean bSuccess = false;
        private SocketSearchThread(){

        }

        @Override
        public void run() {
            super.run();

            try {
                DatagramSocket bcSocket = new DatagramSocket(null);
                bcSocket.setReuseAddress(true);
                bcSocket.bind(new InetSocketAddress(6789));
                bcSocket.setSoTimeout(1000);

                String strSearch = "{\"type\":255}";
                byte[] bufferSend = strSearch.getBytes("utf-8");
                DatagramPacket sendPacket = new DatagramPacket(bufferSend, bufferSend.length, InetAddress.getByName("255.255.255.255"), 6789);
                bcSocket.send(sendPacket);

                DatagramPacket receivePacket;
                for (int i = 0; i < 5; i++){
                    byte[] bufferRecv = new byte[256];
                    receivePacket = new DatagramPacket(bufferRecv, bufferRecv.length);
                    receivePacket.setPort(6789);

                    InetAddress address = InetAddress.getByName("255.255.255.255");
                    receivePacket.setAddress(address);

                    bcSocket.receive(receivePacket);
                    Log.i("PCT", new String(receivePacket.getData()));
                    try {
                        JSONObject jsonObject = new JSONObject(new String(receivePacket.getData()).trim());
                        Log.i("PCT", "Command:"+jsonObject.getInt("type"));
                        if(0x80ff == jsonObject.getInt("type")){
                            bSuccess = true;
                            iPort = jsonObject.getInt("port");
                            stringAddress = receivePacket.getAddress().getHostAddress();
                            String strRecv;
                            strRecv = new String(receivePacket.getData()).trim();
                            Log.i("PCT", "Search Response:"+strRecv);
                            Message msgSocket = new Message();
                            msgSocket.what = 0x0f;
                            msgSocket.obj = "address:" + receivePacket.getAddress().getHostAddress() + "; data:" + strRecv;
                            handlerSocketRev.sendMessage(msgSocket);
                            break;
                        }
                    }catch (JSONException e){
                        e.printStackTrace();
                        Message errsmg = new Message();
                        errsmg.what = 0x02;
                        errsmg.obj = e.toString();
                        handlerSocketRev.sendMessage(errsmg);
                    }
                }
                if(!bSuccess){
                    Message errsmg = new Message();
                    errsmg.what = 0x02;
                    errsmg.obj = "未收到响应";
                    handlerSocketRev.sendMessage(errsmg);
                }

                bcSocket.disconnect();
                bcSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                handlerSocketRev.sendMessage(errsmg);
            }
        }
    }
    private class DoorLockSearchThread extends Thread{
        private boolean bSuccess = false;
        private DoorLockSearchThread(){

        }

        @Override
        public void run() {
            super.run();

            try {
                Message msgSocket = new Message();
                msgSocket.what = 0x0a;

                String stringDoorLock = "{\"type\":20,\"sequence\":0}";

                Socket socketHost = new Socket(stringAddress, iPort);
                socketHost.setReuseAddress(true);
                OutputStream sender = socketHost.getOutputStream();
                InputStream receiver = socketHost.getInputStream();
                sender.write(stringDoorLock.getBytes());
                sender.flush();

                byte[] buffer = new byte[1024];
                receiver.read(buffer);
                String stringRecv = new String(buffer).trim();
                Log.i("PCT", stringRecv);

                try {
                    JSONObject jsonObject = new JSONObject(stringRecv);
                    Log.i("PCT", "Command:"+jsonObject.getInt("type"));
                    if(0x8014 == jsonObject.getInt("type")){
                        Log.i("PCT", stringRecv);
                        msgSocket.obj = stringRecv;
                        handlerSocketRev.sendMessage(msgSocket);
                    }
                }catch (JSONException e){
                    e.printStackTrace();
                    Message errsmg = new Message();
                    errsmg.what = 0x02;
                    errsmg.obj = e.toString();
                    handlerSocketRev.sendMessage(errsmg);
                }

                socketHost.close();
            } catch (IOException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                handlerSocketRev.sendMessage(errsmg);
            }
        }
    }
    void mToast(String str, int showTime, Context mContext) {
        Toast toast;
        toast = Toast.makeText(mContext, str, showTime);
        toast.setGravity(Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL , 0, 0);  //设置显示位置
        TextView v = (TextView) toast.getView().findViewById(android.R.id.message);
        v.setTextColor(Color.YELLOW);     //设置字体颜色
        toast.show();
    }
    private class SocketSendThread extends Thread{
        String stringCommand;
        int iResponseCommand;
        private SocketSendThread(String str, int iCommand){
            Log.i("PCT", str);
            stringCommand = str;
            iResponseCommand = iCommand;
        }

        @Override
        public void run() {
            super.run();

            try {
                Message msgSocket = new Message();
                msgSocket.what = 0x01;

                Socket socketHost = new Socket(stringAddress, iPort);
                socketHost.setReuseAddress(true);
                OutputStream sender = socketHost.getOutputStream();
                InputStream receiver = socketHost.getInputStream();
                sender.write(stringCommand.getBytes());
                sender.flush();

                byte[] buffer = new byte[1024];
                receiver.read(buffer);
                String stringRecv = new String(buffer).trim();
                Log.i("PCT", stringRecv);

                try {
                    JSONObject jsonObject = new JSONObject(stringRecv);
                    Log.i("PCT", "Command:"+jsonObject.getInt("type"));
                    if(iResponseCommand == jsonObject.getInt("type")){
                        Log.i("PCT", stringRecv);
                        msgSocket.obj = stringRecv;
                        handlerSocketRev.sendMessage(msgSocket);
                    }
                }catch (JSONException e){
                    e.printStackTrace();
                    Message errsmg = new Message();
                    errsmg.what = 0x02;
                    errsmg.obj = e.toString();
                    handlerSocketRev.sendMessage(errsmg);
                }

                socketHost.close();
            } catch (IOException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                handlerSocketRev.sendMessage(errsmg);
            }
        }
    }
    private class SocketRecvThread extends Thread{
        private Socket socketHost;
        private SocketRecvThread(){}
        @Override
        public void run() {
            super.run();

            try {
                socketHost = new Socket(stringAddress, iPort);
                socketHost.setReuseAddress(true);
                InputStream receiver = socketHost.getInputStream();
                while(true) {
                    byte[] buffer = new byte[1024];
                    receiver.read(buffer);
                    String stringRecv = new String(buffer).trim();
                    Log.i("PCT", stringRecv);

                    Message msgSocket = new Message();
                    try {
                        msgSocket.what = 0x01;
                        JSONObject jsonObject = new JSONObject(stringRecv);
                        int iCmd = jsonObject.getInt("type");
                        Log.i("PCT", "Command:" + iCmd);
                        if (0x00F4 == iCmd) {//报警上报
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "alarm:" + "有人撬门";

                        } else if (0x00F5 == iCmd) {//开门上报
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "report:" + "用户" + jsonObject.getInt("id") + "开门";

                        } else if (0x00F6 == iCmd) {//添加新用户
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "new user:" + "添加了新用户" + jsonObject.getInt("id");

                        } else if (0x00F7 == iCmd) {//删除用户
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "del user:" + "删除了用户" + jsonObject.getInt("id");
                        } else if (0x0018 == iCmd) {
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "上报电量为：" + jsonObject.getInt("power");
                        }
                        handlerSocketRev.sendMessage(msgSocket);
                    } catch (JSONException e) {
                        e.printStackTrace();
                        msgSocket.what = 0x02;
                        msgSocket.obj = "Json格式有误";
                        handlerSocketRev.sendMessage(msgSocket);
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                handlerSocketRev.sendMessage(errsmg);
            }finally {
                try{
                    if(socketHost != null)
                        socketHost.close();
                }catch (IOException ee){
                    ee.printStackTrace();
                }
            }
        }


    }
    private class WifiSettingThread extends Thread{
        String stringCommand;
        int iResponseCommand;
        String string_ip;
        int int_port;
        private WifiSettingThread(String str, int iCommand, String ip, int port){
            Log.i("PCT", str);
            stringCommand = str;
            iResponseCommand = iCommand;
            string_ip = ip;
            int_port = port;
        }

        @Override
        public void run() {
            super.run();

            try {
                Message msgSocket = new Message();
                msgSocket.what = 0x01;
                Log.i("PCT", "connect ap:" + string_ip + Integer.toString(int_port));
                Socket socketHost = new Socket(string_ip, int_port);
                socketHost.setReuseAddress(true);
                OutputStream sender = socketHost.getOutputStream();
                InputStream receiver = socketHost.getInputStream();
                sender.write(stringCommand.getBytes());
                sender.flush();

                byte[] buffer = new byte[1024];
                receiver.read(buffer);
                String stringRecv = new String(buffer).trim();
                Log.i("PCT", stringRecv);

                try {
                    JSONObject jsonObject = new JSONObject(stringRecv);
                    Log.i("PCT", "Command:"+jsonObject.getInt("type"));
                    if(iResponseCommand == jsonObject.getInt("type")){
                        Log.i("PCT", stringRecv);
                        msgSocket.obj = stringRecv;
                        handlerSocketRev.sendMessage(msgSocket);
                    }
                }catch (JSONException e){
                    e.printStackTrace();
                    Message errsmg = new Message();
                    errsmg.what = 0x02;
                    errsmg.obj = e.toString();
                    Log.i("PCT", e.toString());
                    handlerSocketRev.sendMessage(errsmg);
                }

                socketHost.close();
            } catch (IOException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                Log.i("PCT", e.toString());
                handlerSocketRev.sendMessage(errsmg);
            }
        }
    }
    String long2ip(long ip){
        StringBuffer sb=new StringBuffer();
        sb.append(String.valueOf((int)(ip&0xff)));
        sb.append('.');
        sb.append(String.valueOf((int)((ip>>8)&0xff)));
        sb.append('.');
        sb.append(String.valueOf((int)((ip>>16)&0xff)));
        sb.append('.');
        sb.append(String.valueOf((int)((ip>>24)&0xff)));
        return sb.toString();
    }
}
