/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.structs;

import ro.pub.arh.beam.assembler.Assembler;
import ro.pub.arh.beam.assembler.engine.OperandBuilder;
import ro.pub.arh.beam.hardware.Opcodes;

/**
 *
 * @author rhobincu
 */
public class OperandType {

    private DataTypes operandType;
    private int value;
    private int bitCount;
    private Pair[] bitPositionPair;
    private String label;
    private int entityIndex;


    public OperandType(String _operandCode, String _operandFormat, int _entityIndex){
        operandType = getType(_operandFormat);
        entityIndex = _entityIndex;
        if(!_operandCode.matches(operandRegex[getIndexOf(operandType)])){
            throw new RuntimeException("Operands don't match. Expecting " + operandType + " found "+
                    _operandCode);
        }
        String[] _arrowSplit = _operandFormat.split("\\-\\>");
        bitCount = Integer.parseInt(_arrowSplit[0].substring(1, _arrowSplit[0].indexOf(']')));
        bitPositionPair = parseBits(_arrowSplit[1]);
        value = selectValue(_operandCode);
        checkValue();
    }

    public void setValue(int _value){
        value = applyLabelFunction(_value);
        checkValue();
    }

    public int addToInstruction(int _instruction){
        int _bits = 0;
        int _shiftedBits = bitCount;
        for(int i=0; i<bitPositionPair.length; i++){
            _shiftedBits -= bitPositionPair[i].bitCount;
            _bits |= ((value >>> _shiftedBits) & getMask(bitPositionPair[i].bitCount)) << bitPositionPair[i].position;
        }

        return _bits | _instruction;
    }

    public int readAsOpcode(){
        if(operandType.equals(DataTypes.opcode)){
            return value << (16 - bitCount);
        }

        return 0;
    }

    public String readAsLabel(){
        return label;
    }

    private DataTypes getType(String _operandFormat) {
        for(int i=0; i<formatRegex.length; i++){
            if(_operandFormat.matches(formatRegex[i])){
                return DataTypes.values()[i];
            }
        }

        throw new RuntimeException("Unknown operand type definition.");
    }

    private static Pair[] parseBits(String _bitMap) {
        String[] _maps = _bitMap.split(";");
        Pair[] _pairs = new Pair[_maps.length];
        for(int i=0; i<_pairs.length; i++){
            _pairs[i] = new Pair();
            _pairs[i].position = Integer.parseInt(_maps[i].substring(1, _maps[i].indexOf(',')));
            _pairs[i].bitCount = Integer.parseInt(_maps[i].substring(_maps[i].indexOf(',') + 1, _maps[i].indexOf(']')));
        }
        return _pairs;
    }

    private void checkValue() {
        if(getMask(bitCount) < Math.abs(value)){
            throw new RuntimeException("Value " + value + " outside accepted range for type " + operandType + ".");
        }
    }

    private int selectValue(String _operandCode) {
//        System.out.println("Found operand type " + operandType + ": " + _operandCode + "; bit count=" + bitCount);
        switch(operandType){
            case opcode: return Opcodes.getOpcodeByName(_operandCode) >> (16 - bitCount);
            case rID: return OperandBuilder.getRegisterNumber(_operandCode);
            case value: return OperandBuilder.getValue(_operandCode);
            case lvalue:try{
                            return OperandBuilder.getValue(_operandCode);
                        }catch(RuntimeException _re){
                            OperandBuilder.testLabel(_operandCode);
                            label = testforLocal(_operandCode);
                            
                            return 0;
                        }
            case label: OperandBuilder.testLabel(_operandCode);
                        label = testforLocal(_operandCode);
                        return 0;

            default: throw new RuntimeException("Invalid data type.");
        }
    }

    private int applyLabelFunction(int _value) {
        if(label != null && label.indexOf("(") != -1){
            String _function = label.substring(0, label.indexOf("("));
            if(_function.equals("LOW")){
                return _value & 0xFFFF;
            }else if(_function.equals("HIGH")){
                return _value >>> 16;
            }else{
                throw new RuntimeException("Unknown label function " + _function);
            }
        }

        return _value;
    }

    private String testforLocal(String _operandCode) {
        String _label = _operandCode;
        if(_operandCode.indexOf('(') != -1){
            _label = _operandCode.substring(_operandCode.indexOf('(') + 1, _operandCode.indexOf(')'));
        }
        if(_label.matches(Assembler.LOCAL_LABEL_REGEX)){
            _label = "File_" + entityIndex + "_" + _label;
        }
        if(_operandCode.indexOf('(') != -1){
            return _operandCode.substring(0, _operandCode.indexOf('(') + 1) + _label + ")";
        }else{
            return _label;
        }
    }

    private static enum DataTypes {
        opcode,
        rID,
        value,
        lvalue,
        label
    };

    private static String[] operandRegex;
    private static String[] formatRegex;
    static{
        formatRegex = new String[DataTypes.values().length];
        operandRegex = new String[DataTypes.values().length];
        for(int i=0; i<formatRegex.length; i++){
            formatRegex[i]="\\[\\d{1,2}\\]" + DataTypes.values()[i].toString() +
                    "\\-\\>\\[\\d{1,2}\\,\\d{1,2}\\](\\;\\[\\d{1,2}\\,\\d{1,2}\\])*";
            operandRegex[i]=getOperandRegex(i);
        }
    }
    private static int getMask(int _bitCount){
        return (1 << _bitCount) - 1;
    }
    private static String getOperandRegex(int _index) {
        DataTypes _type = DataTypes.values()[_index];
        switch(_type){
            case opcode: return "\\w+";
            case rID: return "[rR]\\d+";
            case value: return "-?[0-9]+|0x[0-9a-fA-F]+";
            case lvalue: return "(-?[0-9]+|0x[0-9a-fA-F]+)|([\\w_]+(\\([\\w_\\.]+([\\+\\-][0-9]+)?\\))?)";
            case label: return "[\\w_]+(\\([\\w_]+\\))?";
        }

        throw new RuntimeException();
    }

    private static int getIndexOf(DataTypes _operandType){
        for(int i=0; i<DataTypes.values().length; i++){
            if(DataTypes.values()[i].equals(_operandType)){
                return i;
            }
        }

        throw new RuntimeException("Invalid enum member.");
    }

static class Pair{
    int position;
    int bitCount;
}

}
