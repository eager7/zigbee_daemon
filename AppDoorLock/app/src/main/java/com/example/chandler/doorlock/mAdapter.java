package com.example.chandler.doorlock;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import java.util.LinkedList;
import java.util.Objects;

/**
 * Created by chandler on 2017/7/24.
 */

public class mAdapter extends BaseAdapter{
    private Context mContext;
    private LinkedList<Data> mData;

    public mAdapter(){}

    public mAdapter(LinkedList<Data> mData, Context mContext){
        this.mData = mData;
        this.mContext = mContext;
    }

    @Override
    public int getCount(){
        return mData.size();
    }

    @Override
    public Object getItem(int position){
        return null;
    }

    @Override
    public long getItemId(int position){
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent){
        convertView = LayoutInflater.from(mContext).inflate(R.layout.item_list, parent, false);
        TextView text_index = convertView.findViewById(R.id.text_index);
        TextView text_content = convertView.findViewById(R.id.text_content);

        text_index.setText(""+mData.get(position).getIndex());
        text_content.setText(mData.get(position).getContent());

        return convertView;
    }

    public void add(Data data){
        if(mData == null){
            mData = new LinkedList<>();
        }
        mData.add(data);
        notifyDataSetChanged();
    }
}
