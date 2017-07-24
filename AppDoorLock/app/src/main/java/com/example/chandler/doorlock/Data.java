package com.example.chandler.doorlock;

/**
 * Created by chandler on 2017/7/24.
 */

public class Data {
    private int Index;
    private String content;

    public Data(){}

    public Data(int Index, String content){
        this.Index = Index;
        this.content = content;
    }

    public int getIndex(){
        return Index;
    }

    public String getContent(){
        return content;
    }

    public void setIndex(int Index){
        this.Index = Index;
    }

    public void setContent(String content){
        this.content = content;
    }
}
