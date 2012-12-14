/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import java.util.Vector;
import ro.pub.arh.beam.hardware.Opcodes;

/**
 *
 * @author Ares
 */
public class Statistics {

    private int threadId;
    private Vector<Pair> instructionOccurences;
    private int jumpOutsideBufferRange;
    
public Statistics(int _threadId){
    threadId = _threadId;
    instructionOccurences = new Vector();
}

public void newJumpOutsideBufferRange(){
    jumpOutsideBufferRange++;
}

public void assertInstruction(short _opcode){
    Pair _pair = lookForOpcode(_opcode);

    if(_pair == null){
        _pair = new Pair();
        _pair.opcode = _opcode;
        _pair.name = Opcodes.getNameByOpcode(_opcode);
        instructionOccurences.add(_pair);
    }

    _pair.occurences++;
}

public int getThreadId(){
    return threadId;
}

public int getJumpOutsideBufferRange() {
    return jumpOutsideBufferRange;
}

public long getOccurencesOfInstruction(short _opcode){
    Pair _pair = lookForOpcode(_opcode);
    return _pair == null ? 0 : _pair.occurences;
}

private Pair lookForOpcode(short _opcode) {
    for(int i=0; i<instructionOccurences.size(); i++){
        if(instructionOccurences.elementAt(i).opcode == _opcode){
            return instructionOccurences.elementAt(i);
        }
    }

    return null;
}

class Pair{
    short opcode;
    String name;
    long occurences;
}

}
