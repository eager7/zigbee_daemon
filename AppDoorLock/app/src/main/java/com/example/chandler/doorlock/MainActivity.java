package com.example.chandler.doorlock;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.view.View;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ListView;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MulticastSocket;
import java.net.NetworkInterface;
import java.net.SocketException;
import android.os.Handler;
import android.os.Message;
import android.graphics.Color;
import android.view.Gravity;
import android.widget.TextView;
import android.widget.Toast;
import org.json.JSONObject;
import org.json.JSONArray;
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
    private int iPort = 0;
    private int iIndex = 1;
    private String stringAddress;
    Handler handlerSocketRev = null;
    mAdapter madapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        textHello = (TextView)findViewById(R.id.hello);
        btnSearch = (Button)findViewById(R.id.search_btn);
        btnPassword = (Button)findViewById(R.id.password_btn);
        Button btnClear = (Button) findViewById(R.id.clear_btn);
        ListView listView = (ListView) findViewById(R.id.list_view);
        List<Data> mData = new LinkedList<Data>();
        mData.add(new Data(iIndex++, "暂无数据"));
        for (int i = 0; i < mData.size(); i++){
            Log.i("PCT", mData.get(i).getIndex()+ mData.get(i).getContent());
        }
        madapter = new mAdapter((LinkedList<Data>) mData, MainActivity.this);
        listView.setAdapter(madapter);

        btnSearch.setVisibility(View.INVISIBLE);
        btnPassword.setVisibility(View.INVISIBLE);
        textHello.setText("欢迎使用拓邦智能门锁演示系统");

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.setDrawerListener(toggle);
        toggle.syncState();

        NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);

        handlerSocketRev = new Handler(){
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                switch (msg.what){
                    case (0x01):{//搜索到主机
                        textHello.setText(msg.obj.toString());
                        madapter.add(new Data(iIndex++, msg.obj.toString()));
                    }break;
                    case (0x02):{
                        textHello.setText(msg.obj.toString());
                    }break;
                    default:
                        break;
                }
            }
        };
        btnClear.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                textHello.setText(" ");
            }
        });

    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
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

        if (id == R.id.nav_camera) {
            // Handle the camera action
            btnSearch.setVisibility(View.VISIBLE);
            btnPassword.setVisibility(View.INVISIBLE);
            textHello.setText("Search Host");

            btnSearch.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    textHello.setText("正在搜索主机...");
                    mToast("searchIotcServer", Toast.LENGTH_SHORT, getApplicationContext());
                    Log.i("PCT", "Start Search Thread");
                    new SocketSearchThread().start();
                }
            });

        } else if (id == R.id.nav_gallery) {
            textHello.setText("Temporary Password");
            btnSearch.setVisibility(View.INVISIBLE);
            btnPassword.setVisibility(View.VISIBLE);

            btnPassword.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    String stringPassword = "*";

                    HashSet intHash = new HashSet();
                    Random random = new Random();
                    int iNumber = 0;
                    while (iNumber < 4){
                        int iRandom = random.nextInt(10);
                        if(!intHash.contains(iRandom)){
                            intHash.add(iRandom);
                            stringPassword += iRandom;
                            iNumber++;
                        }
                    }
                    stringPassword+="#";
                    textHello.setText("临时密码为："+stringPassword);
                    String stringAddPassword = "{\"type\":240,\"sequence\":0,\"mac\":0,\"id\":1," +
                            "\"available\":1,\"time_start\":0,\"time_end\":1510629973,\"length\":6," +
                            "\"password\":\""+stringPassword+"\"}";

                    new SocketSendThread(stringAddPassword, 0x8000).start();
                }
            });

        } else if (id == R.id.nav_slideshow) {
            textHello.setText("Event Report");
            btnSearch.setVisibility(View.INVISIBLE);
            btnPassword.setVisibility(View.INVISIBLE);

            new SocketRecvThread().start();
        } else if (id == R.id.nav_manage) {
            textHello.setText("Get Records");

        } else if (id == R.id.nav_share) {

        } else if (id == R.id.nav_send) {

        }

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }
    private class SocketSearchThread extends Thread{
        private SocketSearchThread(){

        }

        @Override
        public void run() {
            super.run();

            try {
                DatagramSocket bcSocket = new DatagramSocket(null);
                bcSocket.setReuseAddress(true);
                bcSocket.bind(new InetSocketAddress(6789));
                bcSocket.setSoTimeout(5000);

                String strSearch = "{\"type\":255}";
                byte[] bufferSend = new byte[256];
                bufferSend = strSearch.getBytes("utf-8");
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
                            iPort = jsonObject.getInt("port");
                            stringAddress = receivePacket.getAddress().getHostAddress();
                            String strRecv;
                            strRecv = new String(receivePacket.getData()).trim();
                            Log.i("PCT", "Search Response:"+strRecv);
                            Message msgSocket = new Message();
                            msgSocket.what = 0x01;
                            msgSocket.obj = "address:" + receivePacket.getAddress().getHostAddress() + "; data:" + strRecv;
                            handlerSocketRev.sendMessage(msgSocket);
                            break;
                        }
                    }catch (JSONException e){
                        e.printStackTrace();
                    }
                }

                bcSocket.disconnect();
                bcSocket.close();
            } catch (SocketException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                handlerSocketRev.sendMessage(errsmg);
            } catch (IOException e) {
                e.printStackTrace();
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
        public String stringCommand;
        public int iResponseCommand;
        private SocketSendThread(String str, int iCommand){
            Log.i("PCT", str);
            stringCommand = str;
            iResponseCommand = iCommand;
        }

        @Override
        public void run() {
            super.run();

            try {
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
                    }
                }catch (JSONException e){
                    e.printStackTrace();
                }

                socketHost.close();
            } catch (SocketException e) {
                e.printStackTrace();
                Message errsmg = new Message();
                errsmg.what = 0x02;
                errsmg.obj = e.toString();
                handlerSocketRev.sendMessage(errsmg);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
    private class SocketRecvThread extends Thread{
        private SocketRecvThread(){}
        @Override
        public void run() {
            super.run();
            while(true){
                try {
                    Socket socketHost = new Socket(stringAddress, iPort);
                    socketHost.setReuseAddress(true);
                    InputStream receiver = socketHost.getInputStream();

                    byte[] buffer = new byte[1024];
                    receiver.read(buffer);
                    String stringRecv = new String(buffer).trim();
                    Log.i("PCT", stringRecv);

                    try {
                        Message msgSocket = new Message();
                        msgSocket.what = 0x01;
                        JSONObject jsonObject = new JSONObject(stringRecv);
                        int iCmd = jsonObject.getInt("type");
                        Log.i("PCT", "Command:"+iCmd);
                        if(0x00F4 == iCmd){//报警上报
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "alarm:" + "有人撬门";

                        }else if(0x00F5 == iCmd){//开门上报
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "report:" + "用户"+jsonObject.getInt("id")+"开门";

                        }else if(0x00F6 == iCmd){//添加新用户
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "new user:" + "添加了新用户"+jsonObject.getInt("id");

                        }else if(0x00F7 == iCmd){//删除用户
                            Log.i("PCT", stringRecv);
                            msgSocket.obj = "del user:" + "删除了用户"+jsonObject.getInt("id");
                        }
                        handlerSocketRev.sendMessage(msgSocket);
                    }catch (JSONException e){
                        e.printStackTrace();
                    }

                    socketHost.close();
                } catch (SocketException e) {
                    e.printStackTrace();
                    Message errsmg = new Message();
                    errsmg.what = 0x02;
                    errsmg.obj = e.toString();
                    handlerSocketRev.sendMessage(errsmg);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }
    }
}
