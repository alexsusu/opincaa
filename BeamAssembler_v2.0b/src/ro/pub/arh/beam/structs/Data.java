//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Data -
//
//------------------------------------------------------------------------------
//     $Id: Data.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.structs;

//------------------------------------------------------------------------------
import ubiCORE.jroot.common.utils.Convertor;

//------------------------------------------------------------------------------
public class Data extends Instruction{

    private int data;
    private int byteCount;

//------------------------------------------------------------------------------
public Data(int _data, int _address, int _lineNumber){
    this(_data, _address, _lineNumber, 4);
}

//------------------------------------------------------------------------------
public Data(int _data, int _address, int _lineNumber, int _byteCount){
    super(null, _address, _lineNumber, 0);
    byteCount = _byteCount;
    data = _data;
}

public Data(String _label, int _currentAddress, int _currentLine) {
    this(0, _currentAddress, _currentLine, 4);
    label = _label;
}

//------------------------------------------------------------------------------
@Override
public int getInstruction(){
    return endianness ? data : Convertor.reverseInt(data);
}

//------------------------------------------------------------------------------
@Override
public boolean isAbsoluteJump(){
    return true;
}

//------------------------------------------------------------------------------
@Override
public boolean isCall(){
    return label != null;
}

@Override
public void setLabelAddress(int _address){
    data = _address;
}

public int getByteCount(){
    return byteCount;
}

//void setByte(int _bytePosition, byte _byteValue) {
//    int _newData = ((int)_byteValue & 0xFF) << (_bytePosition * 8);
//    data |= _newData;
//}

public void setData(int _data) {
    data = _data;
}

public void setByteCount(int _newByteCount) {
    byteCount = _newByteCount;
}
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Data.java,java $
