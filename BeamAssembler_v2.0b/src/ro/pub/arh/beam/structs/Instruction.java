//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Instruction -
//
//------------------------------------------------------------------------------
//     $Id: Instruction.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.structs;

//------------------------------------------------------------------------------
import ro.pub.arh.beam.hardware.Opcodes;
import ubiCORE.jroot.common.utils.*;

//------------------------------------------------------------------------------
public class Instruction{

    public static final int     INSTRUCTION_LOCATION_COUNT = 4;
    public static final boolean ENDIAN_LITTLE = true;
    public static final boolean ENDIAN_BIG    = false;

    public static final boolean ABSOLUTE_JUMP = true;
    public static final boolean RELATIVE_JUMP = false;

    protected static boolean endianness;

    private OperandType[] operands;

    private int address;
    private int lineNumber;

    protected String label;
    private boolean absolute;
    private int shiftCount;
    private int entityIndex;


//------------------------------------------------------------------------------
static{
        endianness = ENDIAN_LITTLE;
}

//------------------------------------------------------------------------------
public Instruction(String[] _elements, int _address, int _lineNumber, int _entityIndex){
    address = _address;
    lineNumber = _lineNumber;
    entityIndex = _entityIndex;
    if(_elements != null){
        buildOperands(_elements);
    }
}

//------------------------------------------------------------------------------
public String getLabel(){
    if(label == null){
        return null;
    }else if(label.indexOf("(") == -1){
        return label;
    }else{
        return label.substring(label.indexOf("(") + 1, label.indexOf(")"));
    }
}

//------------------------------------------------------------------------------
public boolean isAbsoluteJump(){
    return absolute;
}

//------------------------------------------------------------------------------
public int getJumpOperandShiftCount(){
    return shiftCount;
}

//------------------------------------------------------------------------------
public int getInstruction(){
int _instruction = 0;
//--
    for(int i=0; i<operands.length; i++){
        _instruction = operands[i].addToInstruction(_instruction);
    }
    return endianness ? _instruction : Convertor.reverseInt(_instruction);
}

//------------------------------------------------------------------------------
public byte[] getBytes(){
    return Convertor.IntToBytes(getInstruction());
}

//------------------------------------------------------------------------------
public byte[] getBytesReversed(){
    return Convertor.IntToBytes(Convertor.reverseInt(getInstruction()));
}

//------------------------------------------------------------------------------
public int getAddress(){
    return address;
}

//------------------------------------------------------------------------------
@Override
public String toString(){
    return Integer.toHexString(address) + ": " + Integer.toHexString(getInstruction());
}

//------------------------------------------------------------------------------
public static void setEndianess(boolean _endianness){
    endianness = _endianness;
}


//------------------------------------------------------------------------------
public int getOpcode(){
    return operands[0].readAsOpcode();
}

//------------------------------------------------------------------------------
public String getOpcodeSemantic(){
    return Opcodes.getNameByOpcode((short)getOpcode());
}

//------------------------------------------------------------------------------
public int getLineNumber() {
    return lineNumber;
}

public boolean isCall(){
    //return operands[0].readAsOpcode() == Opcodes.call;
    return false;
}

//------------------------------------------------------------------------------
private void buildOperands(String[] _elements) {
    operands = new OperandType[_elements.length];
    String[] _format = Opcodes.getFormatByName(_elements[0].trim());
    absolute = true;/*(Opcodes.getOpcodeByName(_elements[0]) == Opcodes.vload) ||
            (Opcodes.getOpcodeByName(_elements[0]) == Opcodes.vlload);*/
    if(_format.length != operands.length){
        throw new RuntimeException("Invalid number of arguments for opcode `" + _elements[0] + "`. "+
                (_format.length - 1) + " expected, " + (operands.length - 1) + " found.");
    }
    for(int i=0; i<_elements.length; i++){
        operands[i] = new OperandType(_elements[i], _format[i], entityIndex);
        label = operands[i].readAsLabel();

    }
}

//------------------------------------------------------------------------------
public void setLabelAddress(int _address) {
    for(int i=0; i<operands.length; i++){
        if(operands[i].readAsLabel() != null){
            operands[i].setValue(_address);
            return;
        }
    }

    throw new RuntimeException("Can't find label operand.");
}

void setOpcode(short _replacement) {
    operands[0].setValue(_replacement);
}

public void offsetAddress(int _offset) {
    address += _offset;
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Instruction.java,java $
