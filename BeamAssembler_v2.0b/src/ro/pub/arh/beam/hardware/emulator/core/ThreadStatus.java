/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import ro.pub.arh.beam.hardware.MachineConstants;

/**
 *
 * @author Ares
 */
public class ThreadStatus {

    public static final int THREAD_STATUS_EMPTY = 0;
    public static final int THREAD_STATUS_READY = 1;
    public static final int THREAD_STATUS_RUNNING = 2;
    public static final int THREAD_STATUS_WAIT_STORE = 3;
    public static final int THREAD_STATUS_WAIT_PROG = 4;
    public static final int THREAD_STATUS_WAIT_DATA = 5;
    public static final int THREAD_STATUS_BOTH = 6;
    public static final int THREAD_STATUS_BEING_REMOVED = 7;

    private static final String[] STATUS_LIST = {
        "Empty",
        "Ready",
        "Running",
        "Wait for Store",
        "Wait for Program",
        "Wait for Data",
        "Wait for Both",
        "Being removed"
    };

    private int status;
    private int threadId;
    private int programCounter;

    private static int currentThreadId = 0;
    static ThreadStatus[] threads;

    private long mailbox;
    private boolean hwInterruptsMasked;
    private boolean swInterruptsMasked;


public ThreadStatus(){
    threadId = (currentThreadId++) % MachineConstants.THREAD_COUNT;
    hwInterruptsMasked = false;
    swInterruptsMasked = false;
}

public long getMailbox(){
    return mailbox;
}

public boolean hwInterruptsMasked(){
    return hwInterruptsMasked;
}

public boolean swInterruptsMasked(){
    return swInterruptsMasked;
}

public void setStatus(int _status){
    status = _status;
}

public int getStatus(){
    return status;
}

public String getStatusAsString(){
    return STATUS_LIST[status];
}

public static ThreadStatus getNextThread(int _currentThread){
    if(threads[_currentThread].getStatus() == ThreadStatus.THREAD_STATUS_RUNNING){
        threads[_currentThread].setStatus(ThreadStatus.THREAD_STATUS_READY);
    }
    
    for(int i=_currentThread + 1; i<threads.length; i++){
        if(threads[i].status == THREAD_STATUS_READY){
            threads[i].setStatus(ThreadStatus.THREAD_STATUS_RUNNING);
            return threads[i];
        }
    }

    for(int i=0; i<_currentThread; i++){
        if(threads[i].status == THREAD_STATUS_READY){
            threads[i].setStatus(ThreadStatus.THREAD_STATUS_RUNNING);
            return threads[i];
        }
    }

    return null;
}

public int getProgramCounter() {
    return programCounter;
}

public void setProgramCounter(int programCounter) {
    this.programCounter = programCounter;
}

public static ThreadStatus getNextThread(){
    for(int i=0; i<threads.length; i++){
        if(threads[i].status == THREAD_STATUS_READY){
            threads[i].setStatus(ThreadStatus.THREAD_STATUS_RUNNING);
            return threads[i];
        }
    }

    return null;
}

public static void setThreadList(ThreadStatus[] _threads) {
    threads = _threads;
}

public int getThreadId() {
    return threadId;
}

public void enableHWInterrupts(boolean _state) {
    hwInterruptsMasked = !_state;
}

public void enableSWInterrupts(boolean _state) {
    swInterruptsMasked = !_state;
}

public void clearSWI() {
    mailbox &= ~4;
}

public void writeMailbox(long _value) {
    mailbox = _value;
}

}
