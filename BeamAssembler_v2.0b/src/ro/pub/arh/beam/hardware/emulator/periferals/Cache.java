//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Cache -
//
//------------------------------------------------------------------------------
//     $Id: Cache.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.hardware.emulator.periferals;

//------------------------------------------------------------------------------
public class Cache{

    public static final int LINE_COUNT = 8;
    public static final int LINE_SIZE = 128;
    public static final int ADDRESS_MASK = 0xFFFFFF80;

    private int lineCount;
    private int lineSize;

    private Integer[] addresses;

//------------------------------------------------------------------------------
public Cache(int _lineCount, int _lineSize){
    lineSize = _lineSize;
    lineCount = _lineCount;
    addresses = new Integer[_lineCount];
}

public Cache(){
    this(LINE_COUNT, LINE_SIZE);
}

private void cacheData(int _address){
    for(int i=0; i<addresses.length; i++){
        if(addresses[i] == null){
            addresses[i] = new Integer(_address & ADDRESS_MASK);
            return;
        }
    }

    addresses[getLru()] = new Integer(_address & ADDRESS_MASK);
}

public boolean isCached(int _address){
    for(int i=0; i<addresses.length; i++){
        if(addresses[i] != null && addresses[i].intValue() == (_address & ADDRESS_MASK)){
            return true;
        }
    }

    cacheData(_address);
    return false;
}

private int getLru(){
    return 0;
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Cache.java,java $
