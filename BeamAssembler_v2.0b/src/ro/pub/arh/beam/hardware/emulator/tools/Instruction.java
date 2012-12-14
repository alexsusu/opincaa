/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.tools;

/**
 *
 * @author rhobincu
 */
public class Instruction {

    public final boolean regOnly;
    public final short opCode;
    public final byte booleanOpCode;
    public final byte booleanOperand;
    public final byte left;
    public final byte right;
    public final byte dest;
    public final short value;

    public final int word;
public Instruction(int _instruction){
    word = _instruction;
    regOnly = (_instruction & 0x80000000) == 0;
    if(regOnly){
        opCode = (short)((_instruction >>> 23) & 0xff);
        right = (byte)((_instruction >>> 10) & 0x1f);
        byte _bInstr = (byte)((_instruction >>> 15) & 0xff);
        if((_bInstr & 0xf0) == 0){
            booleanOpCode = _bInstr;
            booleanOperand = 0;
        }else{
            booleanOpCode = (byte)(_bInstr & 0xf0);
            booleanOperand = (byte)(_bInstr & 0x0f);
        }
        value = 0;
    }else{
        opCode = (short)((_instruction >>> 26 << 3) & 0x1ff);
        value = (short)((_instruction >>> 10) & 0xffff);
        booleanOpCode = 0;
        booleanOperand = 0;
        right = 0;
    }
    left = (byte)((_instruction >>> 5) & 0x1f);
    dest = (byte)(_instruction & 0x1f);
}

}
