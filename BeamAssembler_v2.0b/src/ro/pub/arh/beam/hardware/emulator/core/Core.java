/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import ro.pub.arh.beam.hardware.emulator.core.array.ArrayCore;
import ro.pub.arh.beam.hardware.emulator.periferals.Memory;

/**
 *
 * @author Ares
 */
public class Core {
   
    private Memory memory;
    private BeamCore beamCore;
    private ArrayCore arrayCore;

    private volatile boolean runningState;

    public static final int STATUS_DONE = -1;
    public static final int STATUS_TIMEOUT = -2;

public Core(Memory _memory){
    memory = _memory;
    runningState = true;
    arrayCore = new ArrayCore(memory);
    beamCore = new BeamCore(memory, arrayCore);
}

public int start(int _instructionCount){

    if(_instructionCount < 0){
        
        do{
            executeInstruction();
        }while(!stopCriteria() && runningState && beamCore.breakpointHit() == -1);

        runningState = true;
        if(beamCore.breakpointHit() != -1){
             return beamCore.breakpointHit();
        }else{
            return stopCriteria() ? STATUS_DONE : STATUS_TIMEOUT;
        }
    }

    for(int i=0; i<_instructionCount; i++){
        executeInstruction();
        if(stopCriteria() || !runningState || beamCore.breakpointHit() != -1){
            runningState = true;
            if(beamCore.breakpointHit() != -1){
                return beamCore.breakpointHit();
            }else{
                return stopCriteria() ? STATUS_DONE : STATUS_TIMEOUT;
            }
        }
    }

    return STATUS_TIMEOUT;
}

public void stop(){
    runningState = false;
}

public void boot(){
    beamCore.boot();
}

private void executeInstruction() {
    memory.getRTC().count();
    beamCore.executeInstruction();
}

public boolean stopCriteria() {
    return beamCore.stopCriteria();
}

public int getPcForThread(int _threadId) {
    return beamCore.getPcForThread(_threadId);
}

public Statistics[] getBeamStatistics() {
    return beamCore.getStatistics();
}

public boolean addBreakpoint(int _pc) {
    return beamCore.addBreakpoint(_pc);
}

public boolean removeBreakpoint(int _pc) {
    return beamCore.removeBreakpoint(_pc);
}

public int[] readBreakpoints() {
    return beamCore.readBreakpoints();
}

}
