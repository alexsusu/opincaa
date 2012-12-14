/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core.array;

import ro.pub.arh.beam.hardware.emulator.core.BeamRegisterFile;

/**
 *
 * @author rhobincu
 */
public class ArrayRegisterFile {

    public static final int REGISTER_COUNT = 16;
    public static final int REGISTER_BIT_SIZE = 16;

    private Line[] registerFile;

public ArrayRegisterFile(){
    registerFile = new Line[REGISTER_COUNT];
    for(int i=0; i<REGISTER_COUNT; i++){
        registerFile[i] = new Line(Line.TYPE_REG, i);
    }
}

//public void write(int _address, short[] _data){
//    read(_address).setData(_data);
//}

public void write(int _address, Line _data, Line _flag){
    read(_address).setData(_data, _flag);
}

public Line read(int _registerIndex){
    try{
        return registerFile[_registerIndex - BeamRegisterFile.REGISTER_COUNT];
    }catch(ArrayIndexOutOfBoundsException _aiobe){
        throw new RuntimeException("Invalid array register address");
    }
}



}