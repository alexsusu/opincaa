/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.core;

import java.util.Vector;
import ro.pub.arh.beam.hardware.emulator.core.array.ArrayCore;
import ro.pub.arh.beam.hardware.emulator.core.array.ArrayRegisterFile;
import ro.pub.arh.beam.hardware.emulator.core.array.Line;
import ro.pub.arh.beam.hardware.Opcodes;
import ro.pub.arh.beam.execution.gui.ExecutionGui;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.periferals.Memory;
import ro.pub.arh.beam.hardware.emulator.periferals.Peripheral;
import ro.pub.arh.beam.hardware.emulator.tools.Instruction;
import ro.pub.arh.beam.hardware.emulator.tools.Tools;

/**
 *
 * @author rhobincu
 */
public class BeamCore {

    public static final int BREAKPOINT_COUNT = 10;

    private Memory memory;

    private BeamRegisterFile beamRegisterFile;
    private int[] externalReg;

    private ThreadStatus[] threads;
    private DataTransfer dataTransfer;
    private ProgramTransfer programTransfer;
    private ProgramBuffer programBuffer;
    
    private ThreadStatus activeThread;
    private Statistics[] statistics;
    private ArrayCore arrayCore;
    private ArrayRegisterFile arrayRegisterFile;

    private long[] instructionCount;
    private int[] breakpoints;


public BeamCore(Memory _memory, ArrayCore _arrayCore){
    memory = _memory;
    arrayCore = _arrayCore;

    arrayRegisterFile = _arrayCore.getRegisterFile();
    statistics = new Statistics[MachineConstants.THREAD_COUNT];
    beamRegisterFile = new BeamRegisterFile();
    threads = new ThreadStatus[MachineConstants.THREAD_COUNT];
    ThreadStatus.setThreadList(threads);
    programBuffer = new ProgramBuffer();
    programTransfer = new ProgramTransfer(memory, programBuffer, threads, statistics);
    dataTransfer = new DataTransfer(memory, beamRegisterFile, threads, programTransfer);
 
    externalReg = new int[MachineConstants.THREAD_COUNT];

    for(int i=0; i<MachineConstants.THREAD_COUNT; i++){
        threads[i] = new ThreadStatus();
        statistics[i] = new Statistics(i);
    }
    instructionCount = new long[MachineConstants.THREAD_COUNT];
}

public void boot(){
    breakpoints = new int[BREAKPOINT_COUNT];
    for(int i=0; i<instructionCount.length; i++){
        instructionCount[i] = 0;
        ExecutionGui.lastGui.updateCycleCount(i, 0);
    }
    activeThread = threads[0];
    activeThread.setProgramCounter(MachineConstants.BOOT_ADDRESS);
    programTransfer.startTransfer(0, threads[0].getProgramCounter());
    threads[0].setStatus(ThreadStatus.THREAD_STATUS_RUNNING);
    ExecutionGui.lastGui.updateThreadsStatus(threads);
    ExecutionGui.lastGui.updateRegisterFile(beamRegisterFile.copy());

    updateInstructions();
}

public boolean stopCriteria() {

    for(int i=0; i<MachineConstants.THREAD_COUNT; i++){
        //int _instruction = memory.read(getPcForThread(i));
        if((threads[i].getStatus() != ThreadStatus.THREAD_STATUS_EMPTY)){
            return false;
        }
    }
    return true;
}

private void setNewPc(int _newPc){
int _threadId = activeThread.getThreadId();
//--
    activeThread.setProgramCounter(_newPc);
    if(programBuffer.outOfRange(_threadId, _newPc)){
        programTransfer.startTransfer(_threadId, _newPc);
    }
}

private void removeThread() {
    activeThread.setStatus(ThreadStatus.THREAD_STATUS_EMPTY);
}

public void executeInstruction() {

int _threadId;
int _programCounter;
Instruction _instruction = null;
short _opCode = 0;
Line _carry = null;
Line _flag0 = arrayCore.getFlag(ArrayCore.FLAG_0);
//--

    checkHWInterrupts();
    checkSWInterrupts();

    //ExecutionGui.lastGui.updateCycleCount(++instructionCount);
    if(activeThread == null){
        activeThread = ThreadStatus.getNextThread();
        ExecutionGui.lastGui.updateThreadsStatus(threads);
        return;
    }
    ExecutionGui.lastGui.updateCycleCount(activeThread.getThreadId(), ++instructionCount[activeThread.getThreadId()]);
    _threadId = activeThread.getThreadId();
    _programCounter = activeThread.getProgramCounter();

    try {
        _instruction = new Instruction(programBuffer.read(_threadId, _programCounter));
    } catch (OutsideOfBufferRangeException _ex) {
        _ex.printStackTrace();  //this should not happen
    }

    _opCode = _instruction.opCode;
    statistics[_threadId].assertInstruction(_opCode);
    executeBooleanInstruction(_instruction);
    switch(_opCode){
//------------------------------------------------------------------------------
        case Opcodes.move:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.right));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.writeReg(_instruction.dest, beamRegisterFile.read(_threadId, _instruction.right));
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.right);
            }else{
                //System.out.println("Warning! Array to controller moves not implemented in emulator;");
                switch(_instruction.left){
                    case 16: beamRegisterFile.write(_threadId, _instruction.dest, arrayCore.readReg(_instruction.right).getOr0(_flag0)); break; //left = 10000: arrayOut = OR(flag0)
                    case 17: beamRegisterFile.write(_threadId, _instruction.dest, arrayCore.readReg(_instruction.right).getOr(_flag0)); break; //left = 10001: arrayOut = OR(selected)
                    case 19: beamRegisterFile.write(_threadId, _instruction.dest, arrayCore.readReg(_instruction.right).getSum(_flag0)); break; //left = 10100: arrayOut = ADD0(selected)
                    default: throw new RuntimeException("Invalid operation code for ARRAY -> BEAM move.");
                }
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ldix:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, _threadId);
            }else{
                arrayCore.loadIndex(_instruction.dest);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.extget:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, externalReg[_threadId]);
            }else{
                arrayCore.loadExternalReg(_instruction.dest);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ldext:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.left)){
                externalReg[_threadId] = beamRegisterFile.read(_threadId, _instruction.left);
            }else{
                arrayCore.setExternalReg(_instruction.left);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.loadPc:
            setNewPc(_programCounter + 4);
            beamRegisterFile.write(_threadId, _instruction.dest, activeThread.getProgramCounter());
            break;
//------------------------------------------------------------------------------
        case Opcodes.clri:
            setNewPc(_programCounter + 4);
            threads[beamRegisterFile.read(_threadId, _instruction.left)].clearSWI();
            break;
//------------------------------------------------------------------------------
        case Opcodes.shget:
            setNewPc(_programCounter + 4);
            arrayCore.loadShiftReg(_instruction.dest);
            break;
//------------------------------------------------------------------------------
        case Opcodes.load: //dunno what this does, kk?
            System.out.println("Warning! Load instruction not implemented in emulator;");
            setNewPc(_programCounter + 4);
            break;
//------------------------------------------------------------------------------
        case Opcodes.add:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.right) +
                        beamRegisterFile.read(_threadId, _instruction.left));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.setCarry(arrayCore.add(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right), _instruction.dest));
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.setCarry(arrayCore.add(_instruction.left, _instruction.right, _instruction.dest));
            }else{
                System.out.println("Warning! Array to controller adds not implemented in emulator;");
                //TBD
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.sub:
           setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) -
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.setCarry(arrayCore.sub(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right), _instruction.dest));
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.setCarry(arrayCore.sub(_instruction.left, _instruction.right, _instruction.dest));
            }else{
                System.out.println("Warning! Array to controller subs not implemented in emulator;");
                //TBD
            }
            break;

//------------------------------------------------------------------------------
        case Opcodes.inc:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.left) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) + 1);
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.setCarry(arrayCore.add(_instruction.left, (short)1, _instruction.dest));
            }else{
                System.out.println("Warning! Array to controller incs not implemented in emulator;");
                //TBD
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.dec:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.left) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) - 1);
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.setCarry(arrayCore.sub(_instruction.left, (short)1, _instruction.dest));
            }else{
                System.out.println("Warning! Array to controller decs not implemented in emulator;");
                //TBD
            }
            break;

//------------------------------------------------------------------------------
        case Opcodes.shl:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) <<
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).shl(beamRegisterFile.read(_threadId, _instruction.right),_flag0);
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).shl(arrayRegisterFile.read(_instruction.right),_flag0);
            }else{
                System.out.println("Warning! error in shl interpretation");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.shr:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) >>>
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).shr(beamRegisterFile.read(_threadId, _instruction.right),_flag0);
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).shr(arrayRegisterFile.read(_instruction.right),_flag0);
            }else{
                System.out.println("Warning! error in shr interpretation");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ashr:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) >>
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ashr(beamRegisterFile.read(_threadId, _instruction.right),_flag0);
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ashr(arrayRegisterFile.read(_instruction.right),_flag0);
            }else{
                System.out.println("Warning! error in ashr interpretation");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.rot:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.right) && isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, Tools.rotate(beamRegisterFile.read(_threadId, _instruction.left),
                        beamRegisterFile.read(_threadId, _instruction.right), BeamRegisterFile.REGISTER_BIT_SIZE));
            }else if(isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).rot(beamRegisterFile.read(_threadId, _instruction.right),_flag0);
            }else if(!isControllerRegister(_instruction.right) && !isControllerRegister(_instruction.dest)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).rot(arrayRegisterFile.read(_instruction.right),_flag0);
            }else{
                System.out.println("Warning! error in rot interpretation");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ishl:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) << _instruction.right);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).shl(_instruction.right,_flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ishr:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) >>> _instruction.right);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).shr(_instruction.right,_flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.iashr:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) >> _instruction.right);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ashr(_instruction.right,_flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.irot:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, Tools.rotate(beamRegisterFile.read(_threadId, _instruction.left), _instruction.right, BeamRegisterFile.REGISTER_BIT_SIZE));
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).rot(_instruction.right,_flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ixadd:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.right) + _threadId);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.right);
                arrayCore.setCarry(arrayRegisterFile.read(_instruction.dest).add(Line.index,_flag0));
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ixsub:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, _threadId - beamRegisterFile.read(_threadId, _instruction.right));
            }else{
                arrayCore.loadIndex(_instruction.dest);
                arrayCore.setCarry(arrayRegisterFile.read(_instruction.dest).sub(arrayRegisterFile.read(_instruction.right),_flag0));
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ixcradd:
            setNewPc(_programCounter + 4);
            arrayCore.loadIndex(_instruction.dest);
            _carry = arrayRegisterFile.read(_instruction.dest).add(arrayRegisterFile.read(_instruction.right),_flag0);
            _carry = arrayRegisterFile.read(_instruction.dest).add(_carry,_flag0);
            arrayCore.setCarry(_carry);
            break;
//------------------------------------------------------------------------------
        case Opcodes.ixcrsub:
            setNewPc(_programCounter + 4);
            arrayCore.loadIndex(_instruction.dest);
            _carry = arrayRegisterFile.read(_instruction.dest).sub(arrayRegisterFile.read(_instruction.right),_flag0);
            _carry = arrayRegisterFile.read(_instruction.dest).sub(_carry,_flag0);
            arrayCore.setCarry(_carry);
            break;
//------------------------------------------------------------------------------
        case Opcodes.bwnot:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, ~beamRegisterFile.read(_threadId, _instruction.left));
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).not(_flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.bwand:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) &
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).and(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).and(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.bwxor:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) ^
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).xor(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).xor(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.bwor:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) |
                        beamRegisterFile.read(_threadId, _instruction.right));
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).or(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).or(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.abs:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, Math.abs(beamRegisterFile.read(_threadId, _instruction.left)));
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).abs(_flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.div:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) / beamRegisterFile.read(_threadId, _instruction.right) );
                beamRegisterFile.writeExt(_threadId, beamRegisterFile.read(_threadId, _instruction.left) % beamRegisterFile.read(_threadId, _instruction.right) );
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).div(arrayRegisterFile.read(_instruction.right), arrayCore.getExternalReg(), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).div(beamRegisterFile.read(_threadId, _instruction.right), arrayCore.getExternalReg(), _flag0);
            }
            break;
        case Opcodes.mult0:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) * beamRegisterFile.read(_threadId, _instruction.right) );
                beamRegisterFile.writeExt(_threadId, (int)(((long)beamRegisterFile.read(_threadId, _instruction.left) * (long)beamRegisterFile.read(_threadId, _instruction.right)) >>> 32) );
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).mul(arrayRegisterFile.read(_instruction.right), arrayCore.getExternalReg(), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).mul(beamRegisterFile.read(_threadId, _instruction.right), arrayCore.getExternalReg(), _flag0);
            }
            break;
        case Opcodes.mult1:
            setNewPc(_programCounter + 4);
            System.out.print("Warning!!! Mult1 should not appear in instruction pipeline!!!");
            break;
        case Opcodes.mult2:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) * beamRegisterFile.read(_threadId, _instruction.right) );
                beamRegisterFile.writeExt(_threadId, (int)((((long)beamRegisterFile.read(_threadId, _instruction.left) & 0xffffffff) * ((long)beamRegisterFile.read(_threadId, _instruction.right) & 0xffffffff)) >>> 32) );
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).umul(arrayRegisterFile.read(_instruction.right), arrayCore.getExternalReg(), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).umul(beamRegisterFile.read(_threadId, _instruction.right), arrayCore.getExternalReg(), _flag0);
            }
            break;
        case Opcodes.mult3:
        case Opcodes.mult4:
            setNewPc(_programCounter + 4);
            System.out.print("Warning!!! Mult3/4 should not appear in instruction pipeline!!!");
            break;
//------------------------------------------------------------------------------
        case Opcodes.write:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.left)){
                if(isControllerRegister(_instruction.right)){
                    dataTransfer.store(_threadId, beamRegisterFile.read(_threadId, _instruction.left), _instruction.right);
                }else{
                    arrayCore.memWrite(beamRegisterFile.read(_threadId, _instruction.left), arrayRegisterFile.read(_instruction.right));
                }
            }else{
                System.out.println("Warning! Random line writes in array data mem not implemented. Are we sure we need this?");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.read:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.left)){
                if(isControllerRegister(_instruction.dest)){
                    dataTransfer.load(_threadId, beamRegisterFile.read(_threadId, _instruction.left), _instruction.dest);
                }else{
                    arrayCore.memRead(beamRegisterFile.read(_threadId, _instruction.left), arrayRegisterFile.read(_instruction.dest));
                }
            }else{
                System.out.println("Warning! Random line read from array data mem not implemented. Are we sure we need this?");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.cas:
            setNewPc(_programCounter + 4);
            int _value = memory.read(beamRegisterFile.read(_threadId, _instruction.left));
            if(_value == beamRegisterFile.read(_threadId, _instruction.right)){
                memory.write(beamRegisterFile.read(_threadId, _instruction.left), beamRegisterFile.read(_threadId, _instruction.dest));
            }
            beamRegisterFile.write(_threadId, _instruction.dest, _value);
            
            break;
//------------------------------------------------------------------------------
        case Opcodes.loadm:
            setNewPc(_programCounter + 4);
            System.out.println("Warning! loadm insn not implemented.");
            break;
//------------------------------------------------------------------------------
        case Opcodes.byterd:
            setNewPc(_programCounter + 4);
            dataTransfer.loadByte(_threadId, beamRegisterFile.read(_threadId, _instruction.left), _instruction.dest);
            break;
//------------------------------------------------------------------------------
        case Opcodes.bytewr:
            setNewPc(_programCounter + 4);
            dataTransfer.storeByte(_threadId, beamRegisterFile.read(_threadId, _instruction.left), _instruction.right);
            break;
//------------------------------------------------------------------------------
        case Opcodes.srtrd:
            setNewPc(_programCounter + 4);
            dataTransfer.loadShort(_threadId, beamRegisterFile.read(_threadId, _instruction.left), _instruction.dest);
            break;
//------------------------------------------------------------------------------
        case Opcodes.srtwr:
            setNewPc(_programCounter + 4);
            dataTransfer.storeShort(_threadId, beamRegisterFile.read(_threadId, _instruction.left), _instruction.right);
            break;
//------------------------------------------------------------------------------
        case Opcodes.irqret:
            activeThread.enableHWInterrupts(true);
            setNewPc(beamRegisterFile.read(_threadId, _instruction.left));
            break;
//------------------------------------------------------------------------------
        case Opcodes.shwait:
            setNewPc(_programCounter + 4);
            //System.out.println("SHWAIT acts as nop in emulator.");
            break;
//------------------------------------------------------------------------------
        case Opcodes.endSub:
            throw new RuntimeException("Error!! endsub found in pipeline!");
//------------------------------------------------------------------------------
        case Opcodes.vwait:
            setNewPc(_programCounter + 4);
            System.out.println("VWAIT acts as nop in emulator.");
            break;
//------------------------------------------------------------------------------
        case Opcodes.remTh:
            setNewPc(_programCounter + 4);
            removeThread();
            break;
//------------------------------------------------------------------------------
        case Opcodes.swi:
            setNewPc(_programCounter + 4);
            activeThread.writeMailbox((long)beamRegisterFile.read(_threadId, _instruction.right) << 32
                    | (long)(beamRegisterFile.read(_threadId, _instruction.left) & 0x0ffffffff));
            break;
//------------------------------------------------------------------------------            
        case Opcodes.rjmp:
            setNewPc(_programCounter + beamRegisterFile.read(_threadId, _instruction.left));
            break;
//------------------------------------------------------------------------------
        case Opcodes.ajmp:
            setNewPc(beamRegisterFile.read(_threadId, _instruction.left));
            break;
//------------------------------------------------------------------------------
        case Opcodes.srcreg:
            setNewPc(_programCounter + 4);
            arrayCore.startSearch(_instruction.left, _instruction.right);
            break;
//------------------------------------------------------------------------------
        case Opcodes.swiret:
            activeThread.enableSWInterrupts(true);
            setNewPc(beamRegisterFile.read(_threadId, _instruction.left));
            break;
//------------------------------------------------------------------------------
        case Opcodes.lnot:
            setNewPc(_programCounter + 4);
            arrayCore.moveReg(_instruction.dest, _instruction.left);
            arrayCore.readReg(_instruction.dest).lnot(_flag0);
            break;
//------------------------------------------------------------------------------
        case Opcodes.land:
            setNewPc(_programCounter + 4);
            arrayCore.moveReg(_instruction.dest, _instruction.left);
            arrayCore.readReg(_instruction.dest).land(arrayCore.readReg(_instruction.right), _flag0);
            break;
//------------------------------------------------------------------------------
        case Opcodes.lor:
            setNewPc(_programCounter + 4);
            arrayCore.moveReg(_instruction.dest, _instruction.left);
            arrayCore.readReg(_instruction.dest).lor(arrayCore.readReg(_instruction.right), _flag0);
            break;
//------------------------------------------------------------------------------
        case Opcodes.insert:
            setNewPc(_programCounter + 4);
            System.out.println("Warning! insert insn not implemented (acts as nop).");
            break;
//------------------------------------------------------------------------------
        case Opcodes.delete:
            setNewPc(_programCounter + 4);
            System.out.println("Warning! delete insn not implemented (acts as nop).");
            break;
//------------------------------------------------------------------------------
        case Opcodes.cry:
            setNewPc(_programCounter + 4);
            long _left = beamRegisterFile.read(_threadId, _instruction.left) & 0xffffffff;
            long _right = beamRegisterFile.read(_threadId, _instruction.right) & 0xffffffff;
            beamRegisterFile.write(_threadId, _instruction.dest, _left + _right > 0xffffffffL ? 1 : 0);
            break;
//------------------------------------------------------------------------------
        case Opcodes.brw:
            setNewPc(_programCounter + 4);
            _left = beamRegisterFile.read(_threadId, _instruction.left) & 0xffffffff;
            _right = beamRegisterFile.read(_threadId, _instruction.right) & 0xffffffff;
            beamRegisterFile.write(_threadId, _instruction.dest, _left - _right > 0xffffffffL ? 1 : 0);
            break;
//------------------------------------------------------------------------------
        case Opcodes.eq:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                 beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) == beamRegisterFile.read(_threadId, _instruction.right) ? 1 : 0);
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).eq(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).eq(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ult:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                 beamRegisterFile.write(_threadId, _instruction.dest, ((long)beamRegisterFile.read(_threadId, _instruction.left) & 0xffffffff) <
                         ((long)beamRegisterFile.read(_threadId, _instruction.right) & 0xffffffff) ? 1 : 0);
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ult(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ult(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ugt:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                 beamRegisterFile.write(_threadId, _instruction.dest, ((long)beamRegisterFile.read(_threadId, _instruction.left) & 0xffffffff) >
                         ((long)beamRegisterFile.read(_threadId, _instruction.right) & 0xffffffff) ? 1 : 0);
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ugt(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).ugt(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.lt:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) < beamRegisterFile.read(_threadId, _instruction.right) ? 1 : 0);
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).lt(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).lt(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.gt:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) > beamRegisterFile.read(_threadId, _instruction.right) ? 1 : 0);
            }else if(!isControllerRegister(_instruction.right)){
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).gt(arrayRegisterFile.read(_instruction.right), _flag0);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).gt(beamRegisterFile.read(_threadId, _instruction.right), _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.vload:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, _instruction.value);
            }else{
                arrayRegisterFile.read(_instruction.dest).setData(_instruction.value, _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.vlload:
            beamRegisterFile.write(_threadId, _instruction.dest, (beamRegisterFile.read(_threadId, _instruction.left) << 16) | (_instruction.value & 0xffff));
            setNewPc(_programCounter + 4);
            break;
//------------------------------------------------------------------------------
        case Opcodes.insval:
            setNewPc(_programCounter + 4);
            System.out.println("Warning! insval insn not implemented (acts as nop).");
            break;
//------------------------------------------------------------------------------
        case Opcodes.srcval:
            setNewPc(_programCounter + 4);
            System.out.println("Warning! srcval insn not implemented (acts as nop).");
            break;
//------------------------------------------------------------------------------
        case Opcodes.mrc:
            setNewPc(_programCounter + 4);
            switch(_instruction.value){
                case 0x10:
                    threads[_threadId].enableHWInterrupts(true);
                    break;
                case 0x11:
                    threads[_threadId].enableHWInterrupts(false);
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                    break;
                default:
                    throw new RuntimeException("Invalid MRC code " + _instruction.value +"!");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.mcr:
            setNewPc(_programCounter + 4);
            switch(_instruction.value){
                case 1:
                    beamRegisterFile.write(_threadId, _instruction.dest, threads[_threadId].hwInterruptsMasked() ? 1 : 0);
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    beamRegisterFile.write(_threadId, _instruction.dest, 0);
                    System.out.println("MCR code for statistics. Returned 0.");
                    break;
                default:
                    throw new RuntimeException("Invalid MCR code " + _instruction.value +"!");
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.iadd:
             setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) +
                        _instruction.value);
            }else{
                arrayCore.setCarry(arrayCore.add(_instruction.left, _instruction.value, _instruction.dest));
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.rirhi:
            setNewPc(_programCounter + 4);
            beamRegisterFile.write(_threadId, _instruction.dest, (int)(threads[_instruction.value].getMailbox() >>> 32));
            break;
//------------------------------------------------------------------------------
        case Opcodes.rirlo:
            setNewPc(_programCounter + 4);
            beamRegisterFile.write(_threadId, _instruction.dest, (int)threads[_instruction.value].getMailbox());
            break;
//------------------------------------------------------------------------------
        case Opcodes.iand:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) & _instruction.value);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).and(_instruction.value, _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ixor:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) ^ _instruction.value);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).xor(_instruction.value, _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ior:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
                beamRegisterFile.write(_threadId, _instruction.dest, beamRegisterFile.read(_threadId, _instruction.left) | _instruction.value);
            }else{
                arrayCore.moveReg(_instruction.dest, _instruction.left);
                arrayRegisterFile.read(_instruction.dest).or(_instruction.value, _flag0);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.call:
            beamRegisterFile.write(_threadId, 15, _programCounter + 4);
            setNewPc(beamRegisterFile.read(_threadId, _instruction.left));
            break;
//------------------------------------------------------------------------------
        case Opcodes.jmp:
            setNewPc(_programCounter + _instruction.value);
            break;
//------------------------------------------------------------------------------
        case Opcodes.jz:
            if(beamRegisterFile.read(_threadId, _instruction.left) == 0){
                setNewPc(_programCounter + _instruction.value);
            }else{
                setNewPc(_programCounter + 4);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.jnz:
            if(beamRegisterFile.read(_threadId, _instruction.left) != 0){
                setNewPc(_programCounter + _instruction.value);
            }else{
                setNewPc(_programCounter + 4);
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.ird:
            setNewPc(_programCounter + 4);
            if(isControllerRegister(_instruction.dest)){
               dataTransfer.load(_threadId, _instruction.value + beamRegisterFile.read(_threadId, _instruction.left), _instruction.dest);
            }else{
               arrayCore.memRead((_instruction.value >>> 3) + beamRegisterFile.read(_threadId, _instruction.left), arrayRegisterFile.read(_instruction.dest));
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.iwr:
            setNewPc(_programCounter + 4);
            if(_instruction.left < BeamRegisterFile.REGISTER_COUNT){
               dataTransfer.store(_threadId, _instruction.value + beamRegisterFile.read(_threadId, _instruction.dest), _instruction.left);
            }else{
               arrayCore.memWrite((_instruction.value >>> 3) + beamRegisterFile.read(_threadId, _instruction.dest), arrayRegisterFile.read(_instruction.left));
            }
            break;
//------------------------------------------------------------------------------
        case Opcodes.lrot:
            setNewPc(_programCounter + 4);
            arrayCore.lrot(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right));
            break;
//------------------------------------------------------------------------------
        case Opcodes.rrot:
            setNewPc(_programCounter + 4);
            arrayCore.rrot(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right));
            break;
//------------------------------------------------------------------------------
        case Opcodes.lshz:
            setNewPc(_programCounter + 4);
            arrayCore.lshz(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right));
            break;
//------------------------------------------------------------------------------
        case Opcodes.rshz:
            setNewPc(_programCounter + 4);
            arrayCore.rshz(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right));
            break;
//------------------------------------------------------------------------------
        case Opcodes.lshr:
            setNewPc(_programCounter + 4);
            arrayCore.lshr(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right));
            break;
//------------------------------------------------------------------------------
        case Opcodes.rshr:
            setNewPc(_programCounter + 4);
            arrayCore.rshr(_instruction.left, (short)beamRegisterFile.read(_threadId, _instruction.right));
            break;

//------------------------------------------------------------------------------
        default:
            throw new RuntimeException("Unknown opcode exception! 0x" + Integer.toHexString(_instruction.opCode));
    }
    updateInstructions();
    activeThread = ThreadStatus.getNextThread(activeThread.getThreadId());
    ExecutionGui.lastGui.updateThreadsStatus(threads);

}

public int[][] getRegisterFile(){
    return beamRegisterFile.copy();
}

public ThreadStatus[] getThreadStatus(){
    return threads;
}

public Statistics[] getStatistics() {
    return statistics;
}

public int getPcForThread(int _threadId) {
    return threads[_threadId].getProgramCounter();
}

private void executeBooleanInstruction(Instruction _instruction) {
    switch(_instruction.booleanOpCode){
        case Opcodes.bnop: break;
        case Opcodes.rst: arrayCore.booleanReset(); break;
        case Opcodes.endwhere: arrayCore.endwhere(); break;
        case Opcodes.elsew: arrayCore.elsewhere(); break;
        case Opcodes.wherenz: arrayCore.wherenz(_instruction.booleanOperand); break;
        case Opcodes.wherez: arrayCore.wherez(_instruction.booleanOperand); break;
        default: throw new RuntimeException("Instruction " + Opcodes.getNameByBooleanOpcode(_instruction.booleanOpCode) + " not yet implemented");
    }
}

private boolean isControllerRegister(byte _register) {
    return _register >= 0 && _register < BeamRegisterFile.REGISTER_COUNT;
}

private void updateInstructions() {
    Instruction[] _instructions = new Instruction[threads.length];
    int[] _pc = new int[threads.length];
    for(int i=0; i<threads.length; i++){
        _pc[i] = threads[i].getProgramCounter();
        _instructions[i] = new Instruction(programBuffer.read(i, _pc[i]));
    }

    ExecutionGui.lastGui.updateInstruction(_instructions, _pc);
}

boolean addBreakpoint(int _pc) {
    for(int i=0; i<BREAKPOINT_COUNT; i++){
        if(breakpoints[i] == 0){
            breakpoints[i] = _pc;
            System.out.println("Breakpoint " + i + " set on pc 0x" + Integer.toHexString(_pc));
            return true;
        }
    }

    return false;
}

boolean removeBreakpoint(int _pc) {
    for(int i=0; i<BREAKPOINT_COUNT; i++){
        if(breakpoints[i] == _pc){
            breakpoints[i] = 0;
            System.out.println("Breakpoint " + i + " removed from pc 0x" + Integer.toHexString(_pc));
            return true;
        }
    }

    return false;
}

int[] readBreakpoints() {
    return breakpoints;
}

public int breakpointHit() {
    for(int i=0; i<threads.length; i++){
        if(threads[i].getProgramCounter() != 0){
            for(int j=0; j<breakpoints.length; j++){
                if(threads[i].getProgramCounter() == breakpoints[j]){
                    return j;
                }
            }
        }
    }

    return -1;
}

private void checkHWInterrupts() {
    Vector<Peripheral> _peripherals = memory.getPeripherals();
    for(int i=0; i<_peripherals.size(); i++){
        Peripheral _peripheral = _peripherals.elementAt(i);
        if(_peripheral.hasInterruptCapabilities() && _peripheral.interruptActive() &&
                activeThread != null && !activeThread.hwInterruptsMasked()){

            activeThread.enableHWInterrupts(false);
            beamRegisterFile.write(activeThread.getThreadId(), MachineConstants.HW_REG_LINK, activeThread.getProgramCounter());
            setNewPc(MachineConstants.HW_IRQ_HANDLER);
            return;
        }
    }
}

private void checkSWInterrupts() {
    for(int i=0; i<threads.length; i++){
        long _mailbox = threads[i].getMailbox();
        if((_mailbox & 4) != 0){
            int _targetThread = (int)(_mailbox & 3);

            if(threads[_targetThread].swInterruptsMasked()){
                break;
            }

            if(threads[_targetThread].getStatus() == ThreadStatus.THREAD_STATUS_EMPTY){
                threads[_targetThread].setStatus(ThreadStatus.THREAD_STATUS_READY);
            }else{
                beamRegisterFile.write(_targetThread, MachineConstants.SW_REG_LINK, threads[_targetThread].getProgramCounter());
            }
            threads[_targetThread].setProgramCounter(MachineConstants.SW_IRQ_HANDLER);
            if(programBuffer.outOfRange(_targetThread, MachineConstants.SW_IRQ_HANDLER)){
                programTransfer.startTransfer(_targetThread, MachineConstants.SW_IRQ_HANDLER);
            }
            threads[_targetThread].enableHWInterrupts(false);
            threads[_targetThread].enableSWInterrupts(false);
        }
    }
}

}
