/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.structs;

import java.util.Vector;
import ro.pub.arh.beam.assembler.structs.LabelList;
import ro.pub.arh.beam.hardware.Opcodes;
import ro.pub.arh.beam.hardware.emulator.tools.LineSeeker;

/**
 *
 * @author rhobincu
 */
public class Function extends Vector<Instruction>{

    private int startingAddress;
    private String functionName;
    private int byteSize;
    private LabelList labelList;
    private LineSeeker lineSeeker;

public Function(String _name, int _startAddress){
    functionName = _name;
    startingAddress = _startAddress;
    byteSize = 0;
    labelList = new LabelList();
}

public LabelList getLabelList(){
    return labelList;
}

public int getSize(){
    return (byteSize + 3) >>> 2 << 2;
}

public int getAddress(){
    return startingAddress;
}

public String getFunctionName(){
    return functionName;
}

public void addAll(Vector<Instruction> _instructions){
    for(int i=0; i<_instructions.size(); i++){
        if((_instructions.elementAt(i).getAddress() & 3) != 0){
            composeData(getDataAtAddress(_instructions.elementAt(i).getAddress() & ~3), (Data)_instructions.elementAt(i));
        }else{
            addElement(_instructions.elementAt(i));
            byteSize += 4;
        }
    }
}

private Data getDataAtAddress(int _address){
    for(int i=0; i<size(); i++){
        if(elementAt(i).getAddress() == _address){
            return (Data)elementAt(i);
        }
    }

    return null;
}

private static void composeData(Data _dataAtAddress, Data _newData) {
    int _newWord = _dataAtAddress.getInstruction() | _newData.getInstruction();
    int _newByteCount = _dataAtAddress.getByteCount() + _newData.getByteCount();
    int _mask = (1 << (_newByteCount * 8)) - 1;

    _dataAtAddress.setData(_newWord | _mask);
    _dataAtAddress.setByteCount(_newByteCount);
}

public void replaceOpcode(short _source, short _replacement) {
    for(int i=0; i<size(); i++){
        if(elementAt(i).getOpcode() == _source){
            elementAt(i).setOpcode(_replacement);
        }
    }
}

public int moveTo(int _startAddress) {
//    System.out.println("Initial address = " + Integer.toHexString(startingAddress));
//    System.out.println("New address = " + Integer.toHexString(_startAddress));
    int _offset = _startAddress - getAddress();
    if(_offset == 0){
        return 0;
    }
    for(int i=0; i<size(); i++){
        elementAt(i).offsetAddress(_offset);
    }

    for(int i=0; i<labelList.size(); i++){
        labelList.elementAt(i).offsetAddress(_offset);
    }

    startingAddress = _startAddress;
    return _offset;
}


}
