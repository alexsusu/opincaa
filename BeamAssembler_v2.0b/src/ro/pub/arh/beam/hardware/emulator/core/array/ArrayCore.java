/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core.array;

import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.periferals.Memory;

/**
 *
 * @author rhobincu
 */
public class ArrayCore {

    public static final int ARRAY_MEMORY_SIZE = 1024;

    public static final int FLAG_0 = 0;
    public static final int FLAG_1 = 1;
    public static final int FLAG_2 = 2;
    public static final int FLAG_3 = 3;
    public static final int FLAG_4 = 4;
    public static final int FLAG_5 = 5;
    public static final int FLAG_6 = 6;
    public static final int FLAG_BACK = 7;
    public static final int FLAG_FIRST = 8;
    public static final int FLAG_ZERO = 9;
    public static final int FLAG_LT = 10;
    public static final int FLAG_EQ = 11;
    public static final int FLAG_CARRY = 12;
    public static final int FLAG_SGN = 13;
    public static final int FLAG_LEFT = 14;
    public static final int FLAG_RIGHT = 15;

    private Memory memory;

    private ArrayRegisterFile arrayRegisterFile;
    private Line externalReg;
    private Line shiftReg;
    private Line flags;
    private Line[] arrayDataMemory;
    private int[] counter;



public ArrayCore(Memory _memory){
    memory = _memory;
    arrayRegisterFile = new ArrayRegisterFile();
    arrayDataMemory = new Line[ARRAY_MEMORY_SIZE];
    for(int i=0; i<ARRAY_MEMORY_SIZE; i++){
        arrayDataMemory[i] = new Line(Line.TYPE_MEM, i);
    }
    externalReg = new Line(Line.TYPE_NONE, 0);
    shiftReg = new Line(Line.TYPE_NONE, 0);
    flags = new Line(Line.TYPE_FLAG, 0);
    counter = new int[MachineConstants.ARRAY_LENGTH];
    memory.getIODMA().assignMemory(arrayDataMemory);
}

public void writeReg(byte _destAddress, int _value) {
    arrayRegisterFile.read(_destAddress).setData((short)_value, getFlag(FLAG_0));
}

public Line readReg(byte _address) {
    return arrayRegisterFile.read(_address);
}

public void moveReg(byte _destAddress, byte _sourceAddress) {
    arrayRegisterFile.write(_destAddress, arrayRegisterFile.read(_sourceAddress), getFlag(FLAG_0));
}

public void loadIndex(byte _destination) {
    arrayRegisterFile.read(_destination).setData(Line.index, getFlag(FLAG_0));
}

public void loadExternalReg(byte _destination) {
    arrayRegisterFile.write(_destination, getExternalReg(), getFlag(FLAG_0));
}

public void setExternalReg(byte _source) {
        getExternalReg().setData(arrayRegisterFile.read(_source), getFlag(FLAG_0));
}

public void loadShiftReg(byte _destination) {
    arrayRegisterFile.write(_destination, shiftReg, getFlag(FLAG_0));
}

public void lrot(byte _source, short _value){
    Line _flag = getFlag(FLAG_0);
    Line _sourceLine = readReg(_source);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.getCell(i) != 0){
            shiftReg.setCell(i, _sourceLine.getCell((i + _value) % MachineConstants.ARRAY_LENGTH));
        }
    }
}

public void rrot(byte _source, short _value){
    Line _flag = getFlag(FLAG_0);
    Line _sourceLine = readReg(_source);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.getCell(i) != 0){
            shiftReg.setCell(i, _sourceLine.getCell((i - _value) % MachineConstants.ARRAY_LENGTH));
        }
    }
}

public void lshz(byte _source, short _value){
    Line _flag = getFlag(FLAG_0);
    Line _sourceLine = readReg(_source);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.getCell(i) != 0){
            shiftReg.setCell(i, i < MachineConstants.ARRAY_LENGTH - _value ? _sourceLine.getCell((i + _value) % MachineConstants.ARRAY_LENGTH) : 0);
        }
    }
}

public void rshz(byte _source, short _value){
    Line _flag = getFlag(FLAG_0);
    Line _sourceLine = readReg(_source);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.getCell(i) != 0){
            shiftReg.setCell(i, i < _value ? 0 : _sourceLine.getCell((i - _value) % MachineConstants.ARRAY_LENGTH));
        }
    }
}

public void lshr(byte _source, short _value){
    Line _flag = getFlag(FLAG_0);
    Line _sourceLine = readReg(_source);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.getCell(i) != 0){
            shiftReg.setCell(i, i < MachineConstants.ARRAY_LENGTH - _value ? _sourceLine.getCell((i + _value) % MachineConstants.ARRAY_LENGTH) : _sourceLine.getCell(MachineConstants.ARRAY_LENGTH - 1));
        }
    }
}

public void rshr(byte _source, short _value){
    Line _flag = getFlag(FLAG_0);
    Line _sourceLine = readReg(_source);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag.getCell(i) != 0){
            shiftReg.setCell(i, i < _value ? _sourceLine.getCell(0) : _sourceLine.getCell((i - _value) % MachineConstants.ARRAY_LENGTH));
        }
    }
}

public Line add(byte _left, byte _right, byte _dest){
    Line _line = new Line(0, 0);
    _line.setData(arrayRegisterFile.read(_left), getFlag(FLAG_0));
    Line _carry = _line.add(arrayRegisterFile.read(_right), getFlag(FLAG_0));
    arrayRegisterFile.write(_dest, _line, getFlag(0));
    return _carry;
}

public Line add(byte _left, short _value, byte _dest){
    Line _line = new Line(0, 0);
    _line.setData(_value, getFlag(FLAG_0));
    Line _carry = _line.add(arrayRegisterFile.read(_left), getFlag(FLAG_0));
    arrayRegisterFile.write(_dest, _line, getFlag(0));
    return _carry;
}

public Line sub(byte _left, byte _right, byte _dest){
    Line _line = new Line(0, 0);
    _line.setData(arrayRegisterFile.read(_left), getFlag(FLAG_0));
    Line _carry = _line.sub(arrayRegisterFile.read(_right), getFlag(FLAG_0));
    arrayRegisterFile.write(_dest, _line, getFlag(0));
    return _carry;
}

public Line sub(byte _left, short _value, byte _dest){
    Line _line = new Line(0, 0);
    _line.setData(_value, getFlag(FLAG_0));
    Line _carry = _line.sub(arrayRegisterFile.read(_left), getFlag(FLAG_0));
    arrayRegisterFile.write(_dest, _line, getFlag(0));
    return _carry;
}

public ArrayRegisterFile getRegisterFile() {
    return arrayRegisterFile;
}

public void setCarry(Line _carry) {
    setFlag(_carry, FLAG_CARRY);
}

public Line getCarry(){
    return getFlag(FLAG_CARRY);
}

public void memWrite(int _address, Line _data) {
    _address >>>= 2;
    try{
        arrayDataMemory[_address].setData(_data, getFlag(FLAG_0));
        ExecutionGui.lastGui.updateArrayMemory(_address, _data);
    }catch(ArrayIndexOutOfBoundsException _aiobe){
        System.out.println("Warning!! Writing outside array memory range, address " + _address);
    }
}

public void memRead(int _address, Line _data) {
    _address >>>= 2;
    try{
        _data.setData(arrayDataMemory[_address], getFlag(FLAG_0));
    }catch(ArrayIndexOutOfBoundsException _aiobe){
        System.out.println("Warning!! Reading from outside array memory range, address " + _address);
    }
}

private void setFlag(Line _flag, int _flagIndex) {
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        short _cell = (short)(flags.getCell(i) & (~(1 << _flagIndex)));
        _cell |= (_flag.getCell(i) & 1) << _flagIndex;
        flags.setCell(i, _cell);
    }
    ExecutionGui.lastGui.updateArrayFlags(flags);
}

public Line getFlag(int _flagIndex) {
    Line _carry = new Line(Line.TYPE_NONE, 0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        _carry.setCell(i, (short) ((flags.getCell(i) >>> _flagIndex) & 1));
    }

    return _carry;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//BOOLEAN OPS
//------------------------------------------------------------------------------
public void booleanReset(){
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        counter[i] = 0;
    }
    Line _tmp = new Line((short)1);
    setFlag(_tmp, FLAG_0);
}

public void endwhere(){
    Line _flag0 = getFlag(FLAG_0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(_flag0.getCell(i) == 0){
            counter[i]--;
        }
        _flag0.setCell(i, (short)(counter[i] == 0 ? 1 : 0));
    }
    setFlag(_flag0, FLAG_0);
}

public void elsewhere(){
    Line _flag0 = getFlag(FLAG_0);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
        if(counter[i] == 0){
            counter[i] = 1;
            _flag0.setCell(i, (short)0);
        }else if(counter[i] == 1){
            counter[i] = 0;
            _flag0.setCell(i, (short)1);
        }
    }
    setFlag(_flag0, FLAG_0);
}

public void wherenz(int _flagIndex){
    Line _flag0 = getFlag(FLAG_0);
    Line _flag = getFlag(_flagIndex);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
       _flag0.setCell(i, (short)(_flag0.getCell(i) & _flag.getCell(i)));
       if(_flag.getCell(i) == 0){
           counter[i]++;
       }
    }
    setFlag(_flag0, FLAG_0);
}

public void wherez(int _flagIndex){
    Line _flag0 = getFlag(FLAG_0);
    Line _flag = getFlag(_flagIndex);
    for(int i=0; i<MachineConstants.ARRAY_LENGTH; i++){
       _flag0.setCell(i, (short)(_flag0.getCell(i) & (1 - _flag.getCell(i))));
       if(_flag.getCell(i) != 0){
           counter[i]++;
       }
    }
    setFlag(_flag0, FLAG_0);
}

public Line getExternalReg() {
    return externalReg;
}

public void startSearch(byte _left, byte _right) {
    throw new UnsupportedOperationException("Not yet implemented");
}

}
