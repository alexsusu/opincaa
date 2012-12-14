/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core.array;

import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.tools.Tools;

/**
 *
 * @author rhobincu
 */
public class Line {

    
    public static final int TYPE_NONE = 0;
    public static final int TYPE_REG = 1;
    public static final int TYPE_MEM = 2;
    public static final int TYPE_FLAG = 3;


    private short[] data;
    private int type;
    private int address;



    public static final Line index;
static{
    index = new Line(0, 0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        index.data[i] = (short)i;
    }
}

public Line(int _type, int _address){
    data = new short[MachineConstants.ARRAY_LENGTH];
    type = _type;
    address = _address;
}

public Line(){
    this(0, 0);
}

public Line(short _value){
    this(0,0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        data[i] = _value;
    }
}

public Line(short[] _array){
    if(_array.length != MachineConstants.ARRAY_LENGTH){
        throw new RuntimeException("Invalid line MachineConstants.ARRAY_LENGTH: " + _array.length);
    }

    data = _array;
}

public short[] getData(){
    short[] _array = new short[MachineConstants.ARRAY_LENGTH];
    System.arraycopy(data, 0, _array, 0, MachineConstants.ARRAY_LENGTH);
    return _array;
}

//public void setData(short[] _data){
//    data = _data;
//    updateGUI();
//}

public void setData(short _data, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = _data;
        }
    }
    updateGUI();
}

public void setData(Line _anotherLine, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = _anotherLine.data[i];
        }
    }
    updateGUI();
}

public int getMax(Line _flag){
    int _max = Integer.MIN_VALUE;
    for(int i=0; i<data.length; i++){
        if(data[i] > _max && _flag.data[i] != 0){
            _max = data[i];
        }
    }

    return _max;
}

public int getMin(Line _flag){
    int _min = Integer.MAX_VALUE;
    for(int i=0; i<data.length; i++){
        if(data[i] < _min && _flag.data[i] != 0){
            _min = data[i];
        }
    }

    return _min;
}

public int getSum(Line _flag){
    int _sum = 0;
    for(int i=0; i<data.length; i++){
        if(_flag.data[i] != 0){
            _sum += data[i];
        }
    }

    return _sum;
}

public int getOr(Line _flag){
    int _or = 0;
    for(int i=0; i<data.length; i++){
        if(_flag.data[i] != 0){
            _or |= data[i];
        }
    }

    return _or;
}

public int getOr0(Line _flag){
    int _or = 0;
    for(int i=0; i<data.length; i++){
        if(_flag.data[i] != 0){
            _or |= data[i];
        }
    }

    return _or;
}

public Line add(Line _anotherLine, Line _flag){
    Line _carry = new Line(TYPE_FLAG, 0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = data[i] + _anotherLine.data[i];
            data[i] = (short)_newData;
            _newData &= 0xffff8000;
            _carry.data[i] = (_newData  == 0xffff8000 || _newData == 0)? (short)0 : (short)1;
        }
    }
    updateGUI();
    return _carry;
}

public void mul(int _value, Line _ext, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = data[i] * _value;
            data[i] = (short)_newData;
            _ext.data[i] = (short)(_newData >>> 16);
        }
    }
    updateGUI();
}

public void mul(Line _anotherLine, Line _ext, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = data[i] * _anotherLine.data[i];
            data[i] = (short)_newData;
            _ext.data[i] = (short)(_newData >>> 16);
        }
    }
    updateGUI();
}

public void umul(int _value, Line _ext, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = ((int)data[i] & 0xffff) * _value;
            data[i] = (short)_newData;
            _ext.data[i] = (short)(_newData >>> 16);
        }
    }
    updateGUI();
}

public void umul(Line _anotherLine, Line _ext, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = ((int)data[i] & 0xffff) * ((int)_anotherLine.data[i] & 0xffff);
            data[i] = (short)_newData;
            _ext.data[i] = (short)(_newData >>> 16);
        }
    }
    updateGUI();
}

public void div(Line _anotherLine, Line _ext, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (short)(data[i] / _anotherLine.data[i]);
            _ext.data[i] = (short)(data[i] % _anotherLine.data[i]);
        }
    }
    updateGUI();
}

public void div(int _value, Line _ext, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (short)(data[i] / _value);
            _ext.data[i] = (short)(data[i] % _value);
        }
    }
    updateGUI();
}

public Line sub(Line _anotherLine, Line _flag){
    Line _carry = new Line(TYPE_FLAG, 0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = data[i] - _anotherLine.data[i];
            data[i] = (short)_newData;
            _carry.data[i] = ((_newData & 0xffff0000) != 0 && (_newData & 0xffff0000) != 0xffff0000) ? (short)1 : (short)0;
        }
    }
    updateGUI();
    return _carry;
}

public Line sub(int _value, Line _flag){
    Line _carry = new Line(TYPE_FLAG, 0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            int _newData = data[i] - _value;
            data[i] = (short)_newData;
            _carry.data[i] = ((_newData & 0xffff0000) != 0 && (_newData & 0xffff0000) != 0xffff0000) ? (short)1 : (short)0;
        }
    }
    updateGUI();
    return _carry;
}

public void shl(int _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] <<= _shift;
        }
    }
    updateGUI();
}

public void shl(Line _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] <<= _shift.data[i];
        }
    }
    updateGUI();
}

public void shr(int _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] >>>= _shift;
        }
    }
    updateGUI();
}

public void shr(Line _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] >>>= _shift.data[i];
        }
    }
    updateGUI();
}

public void ashr(int _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] >>= _shift;
        }
    }
    updateGUI();
}

public void ashr(Line _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] >>= _shift.data[i];
        }
    }
    updateGUI();
}

public void rot(int _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (short)Tools.rotate(data[i], _shift, ArrayRegisterFile.REGISTER_BIT_SIZE);
        }
    }
    updateGUI();
}

public void rot(Line _shift, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (short)Tools.rotate(data[i], _shift.data[i], ArrayRegisterFile.REGISTER_BIT_SIZE);
        }
    }
    updateGUI();
}

public void not(Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (short)~data[i];
        }
    }
    updateGUI();
}

public void and(Line _line, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] &= _line.data[i];
        }
    }
    updateGUI();
}

public void and(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] &= _value;
        }
    }
    updateGUI();
}

public void xor(Line _line, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] ^= _line.data[i];
        }
    }
    updateGUI();
}

public void xor(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] ^= _value;
        }
    }
    updateGUI();
}

public void or(Line _line, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] |= _line.data[i];
        }
    }
    updateGUI();
}

public void or(int _value, Line _flag){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] |= _value;
        }
    }
    updateGUI();
}

public void lnot(Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] != 0 ? (short)0 : (short)1;
        }
    }
    updateGUI();
}

public void land(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] != 0 && _anotherLine.data[i] != 0 ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void lor(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] != 0 || _anotherLine.data[i] != 0 ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void eq(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] == _anotherLine.data[i] ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void eq(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] == _value ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void ult(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = ((int)data[i] & 0xffff) < ((int)_anotherLine.data[i] & 0xffff) ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void ult(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = ((int)data[i] & 0xffff) < ((long)_value & 0xffffffff) ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void ugt(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = ((int)data[i] & 0xffff) > ((int)_anotherLine.data[i] & 0xffff) ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void ugt(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = ((int)data[i] & 0xffff) > ((long)_value & 0xffffffff) ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void lt(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (int)data[i] < _anotherLine.data[i] ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void lt(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] < _value ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void gt(Line _anotherLine, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (int)data[i] > _anotherLine.data[i] ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void gt(int _value, Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = data[i] > _value ? (short)1 : (short)0;
        }
    }
    updateGUI();
}

public void abs(Line _flag) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.data[i] != 0){
            data[i] = (short)Math.abs(data[i]);
        }
    }
    updateGUI();
}

public short getCell(int _index){
    return data[_index];
}

public void updateGUI(){

    if(ExecutionGui.lastGui == null){
        return;
    }

    switch(type){
        case TYPE_REG: ExecutionGui.lastGui.updateArrayRegister(address, this); return;
        case TYPE_MEM: ExecutionGui.lastGui.updateArrayMemory(address, this); return;
    }
}

public void setCell(int _index, short _cell) {
    data[_index] = _cell;
}


}