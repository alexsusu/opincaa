//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Opcodes -
//
//------------------------------------------------------------------------------
//     $Id: Opcodes.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.hardware;

//------------------------------------------------------------------------------
public class Opcodes{

    public static final short nop      = (short)0x00;
//    public static final short ldix     = (short)0x01;
//    public static final short extget   = (short)0x02;
//    public static final short ldext    = (short)0x03;
//    public static final short loadPc   = (short)0x04;
//    public static final short clri     = (short)0x05;
//    public static final short shget    = (short)0x06;
//    public static final short load     = (short)0x07;
//    public static final short add      = (short)0x08;
//    public static final short sub      = (short)0x09;
//    public static final short cradd    = (short)0x0A;
//    public static final short crsub    = (short)0x0B;
//    public static final short inc      = (short)0x0C;
//    public static final short dec      = (short)0x0D;
//    public static final short crinc    = (short)0x0E;
//    public static final short crdec    = (short)0x0F;
//    public static final short shl      = (short)0x10;
//    public static final short shr      = (short)0x11;
//    public static final short ashr     = (short)0x12;
//    public static final short rot      = (short)0x13;
//    public static final short ishl     = (short)0x14;
//    public static final short ishr     = (short)0x15;
//    public static final short iashr    = (short)0x16;
//    public static final short irot     = (short)0x17;
//    public static final short ixadd    = (short)0x18;
//    public static final short ixsub    = (short)0x19;
//    public static final short ixcradd  = (short)0x1A;
//    public static final short ixcrsub  = (short)0x1B;
//    public static final short bwnot    = (short)0x1C;
//    public static final short bwand    = (short)0x1D;
//    public static final short bwxor    = (short)0x1E;
//    public static final short bwor     = (short)0x1F;
//    public static final short abs      = (short)0x20;
//    public static final short div      = (short)0x21;
//    public static final short modulo   = (short)0x22;
//    public static final short mult4    = (short)0x23;
//    public static final short mult0    = (short)0x24;
//    public static final short mult1    = (short)0x25;
//    public static final short mult2    = (short)0x26;
//    public static final short mult3    = (short)0x27;
//    public static final short write    = (short)0x28;
//    public static final short read     = (short)0x29;
//    public static final short cas      = (short)0x2A;
//    public static final short loadm    = (short)0x2B;
//    public static final short byterd   = (short)0x2C;
//    public static final short bytewr   = (short)0x2D;
//    public static final short srtrd    = (short)0x2E;
//    public static final short srtwr    = (short)0x2F;
//    public static final short unused   = (short)0x30;
//    public static final short unused   = (short)0x31;
    public static final short iwr      = (short)0x32;
//    public static final short unused   = (short)0x33;
    public static final short ird      = (short)0x34;
    public static final short vload    = (short)0x35;
//    public static final short unused   = (short)0x36;
//    public static final short unused   = (short)0x37;
//    public static final short irqret   = (short)0x38;
//    public static final short shwait   = (short)0x39;
//    public static final short endSub   = (short)0x3A;
//    public static final short vwait    = (short)0x3B;
//    public static final short remTh    = (short)0x3C;
//    public static final short swi      = (short)0x3D;
//    public static final short rjmp     = (short)0x3E;
//    public static final short ajmp     = (short)0x3F;
//    public static final short srcreg   = (short)0x40;
//    public static final short swiret   = (short)0x41;
//    public static final short srcscal  = (short)0x42;
//    public static final short land     = (short)0x43;
//    public static final short lnot     = (short)0x44;
//    public static final short land     = (short)0x45;
//    public static final short unused   = (short)0x46;
//    public static final short lor      = (short)0x47;
//    public static final short unused   = (short)0x48;
//    public static final short unused   = (short)0x49;
//    public static final short unused   = (short)0x4A;
//    public static final short unused   = (short)0x4B;
//    public static final short unused   = (short)0x4C;
//    public static final short unused   = (short)0x4D;
//    public static final short unused   = (short)0x4E;
//    public static final short unused   = (short)0x4F;
//    public static final short radd     = (short)0x50;
//    public static final short rsub     = (short)0x51;
//    public static final short rcradd   = (short)0x52;
//    public static final short rcrsub   = (short)0x53;
//    public static final short rbwnot   = (short)0x54;
//    public static final short rbwand   = (short)0x55;
//    public static final short rbwxor   = (short)0x56;
//    public static final short rbwor    = (short)0x57;
//    public static final short madd     = (short)0x58;
//    public static final short msub     = (short)0x59;
//    public static final short mcradd   = (short)0x5A;
//    public static final short mcrsub   = (short)0x5B;
//    public static final short mbwnot   = (short)0x5C;
//    public static final short mbwand   = (short)0x5D;
//    public static final short mbwxor   = (short)0x5E;
//    public static final short mbwor    = (short)0x5F;
//    public static final short insert   = (short)0x60;
//    public static final short unused   = (short)0x61;
//    public static final short delete   = (short)0x62;
//    public static final short addsh    = (short)0x63;
//    public static final short lrot     = (short)0x64;
//    public static final short rrot     = (short)0x65;
//    public static final short memlrot  = (short)0x66;
//    public static final short memrrot  = (short)0x67;
//    public static final short lrot     = (short)0x68;
//    public static final short rrot     = (short)0x69;
//    public static final short lshz     = (short)0x6A;
//    public static final short rshz     = (short)0x6B;
//    public static final short lshr     = (short)0x6C;
//    public static final short rshr     = (short)0x6D;
//    public static final short memlshr  = (short)0x6E;
//    public static final short memrshr  = (short)0x6F;
//    public static final short cry      = (short)0x70;
//    public static final short brw      = (short)0x71;
//    public static final short eq       = (short)0x72;
//    public static final short unused   = (short)0x73;
//    public static final short ult      = (short)0x74;
//    public static final short ugt      = (short)0x75;
//    public static final short lt       = (short)0x76;
//    public static final short gt       = (short)0x77;
//    public static final short unused   = (short)0x78;
//    public static final short unused   = (short)0x79;
//    public static final short unused   = (short)0x7A;
    
    public static final short red      = (short)0x100;
    public static final short mult     = (short)0x108;
    public static final short cellshr  = (short)0x111;
    public static final short cellshl  = (short)0x112;
    public static final short write    = (short)0x114;
    public static final short wherecry = (short)0x11C;
    public static final short whereeq  = (short)0x11D;
    public static final short wherelt  = (short)0x11E;
    public static final short endwhere = (short)0x11F;
    public static final short ldix     = (short)0x120;
    public static final short read     = (short)0x124;
    public static final short multl    = (short)0x128;
    public static final short ldsh     = (short)0x130;
    public static final short multh    = (short)0x138;
    public static final short shl      = (short)0x140;
    public static final short ishl     = (short)0x141;
    public static final short add      = (short)0x144;
    public static final short eq       = (short)0x148;
    public static final short lnot     = (short)0x14C;
    public static final short shr      = (short)0x150;
    public static final short ishr     = (short)0x151;
    public static final short sub      = (short)0x154;
    public static final short lt       = (short)0x158;
    public static final short lor      = (short)0x15C;
    public static final short shra     = (short)0x160;
    public static final short ishra    = (short)0x161;
    public static final short addc     = (short)0x164;
    public static final short ult      = (short)0x168;
    public static final short land     = (short)0x16C;
    public static final short subc     = (short)0x174;
    public static final short lxor     = (short)0x17C;

    private static String[] opCodes;
    private static String[] format;

//------------------------------------------------------------------------------
static{
    opCodes = new String[512];

    opCodes[nop]        =           "nop";
    opCodes[iwr]        =           "iwr";
    opCodes[ird]        =           "ird";
    opCodes[vload]      =           "vload";
    opCodes[red]        =           "red";
    opCodes[mult]       =           "mult";
    opCodes[cellshr]    =           "cellshr";
    opCodes[cellshl]    =           "cellshl";
    opCodes[write]      =           "write";
    opCodes[wherecry]   =           "wherecry";
    opCodes[whereeq]    =           "whereeq";
    opCodes[wherelt]    =           "wherelt";
    opCodes[endwhere]   =           "endwhere";
    opCodes[ldix]       =           "ldix";
    opCodes[read]       =           "read";
    opCodes[multl]      =           "multl";
    opCodes[ldsh]       =           "ldsh";
    opCodes[multh]      =           "multh";
    opCodes[shl]        =           "shl";
    opCodes[ishl]       =           "ishl";
    opCodes[add]        =           "add";
    opCodes[eq]         =           "eq";
    opCodes[lnot]       =           "lnot";
    opCodes[shr]        =           "shr";
    opCodes[ishr]       =           "ishr";
    opCodes[sub]        =           "sub";
    opCodes[lt]         =           "lt";
    opCodes[lor]        =           "lor";
    opCodes[shra]       =           "shra";
    opCodes[ishra]      =           "ishra";
    opCodes[addc]       =           "addc";
    opCodes[ult]        =           "ult";
    opCodes[land]       =           "land";
    opCodes[subc]       =           "subc";
    opCodes[lxor]       =           "lxor";

    
    //types can be:
    //rID - register ID;
    //val - immediate value, as decimal or hex number;
    //label - value as label
    //opCode - operation (instruction code) code 
    //---
    /* each format is composed of one or more fields
     * A field has the following format:
     * [field_size]field_type->[chunk1_start_from_bit][bit_chunk_1][chunk2_start_from_bit][bit_chunk2]
     * For example, [16]opcode->[23,16] translates as:
     *   - a 16 bit opcode which is split in 1 chunk (so actually not split) of 16 bits, which is placed at bit 23
     * [16]value->[23,8][0,8] translates as:
     *   - a 16 bit numerical value which is split in 2 chunks, first one (LS) having 8 bits, being placed
     * at bit 23 (so it occupies bits [30-23]), and second one (MS) having 8 bits, and placed at bit 0, thus 
     * occupying bits [7-0].
     * 
     * current implementarion:
     * Left  = [5]rID->[5,5]
     * Right = [5]rID->[10,5]
     * Dest  = [5]rID->[0,5]
     * Value = [16]value->[10,16]
     * 
     * Order is always: right left dest value
     * 
     */
    format = new String[512];


    format[nop]         ="[6]opcode->[26,6]";
    format[iwr]         ="[6]opcode->[26,6] [5]rID->[5,5] [16]value->[10,16]";
    format[ird]         ="[6]opcode->[26,6] [5]rID->[0,5] [16]value->[10,16]";
    format[vload]       ="[6]opcode->[26,6] [5]rID->[0,5] [16]value->[10,16]";
    format[red]         ="[9]opcode->[23,9] [5]rID->[5,5]";
    format[mult]        ="[9]opcode->[23,9] [5]rID->[5,5] [5]rID->[10,5]";
    format[cellshr]     ="[9]opcode->[23,9] [5]rID->[5,5] [5]rID->[10,5]";
    format[cellshl]     ="[9]opcode->[23,9] [5]rID->[5,5] [5]rID->[10,5]";
    format[write]       ="[9]opcode->[23,9] [5]rID->[5,5] [5]rID->[10,5]";
    format[wherecry]    ="[9]opcode->[23,9]";
    format[whereeq]     ="[9]opcode->[23,9]";    
    format[wherelt]     ="[9]opcode->[23,9]";    
    format[endwhere]    ="[9]opcode->[23,9]";        
    format[ldix]        ="[9]opcode->[23,9] [5]rID->[0,5]";
    format[read]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[0,5]";
    format[multl]       ="[9]opcode->[23,9] [5]rID->[0,5]";
    format[ldsh]        ="[9]opcode->[23,9] [5]rID->[0,5]";
    format[multh]       ="[9]opcode->[23,9] [5]rID->[0,5]";
    format[shl]         ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[ishl]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[0,5] [5]value->[5,5]";   //value is last here but in place of the "right" register
    format[add]         ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[eq]          ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[lnot]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[0,5]";
    format[shr]         ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[ishr]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[0,5] [5]value->[5,5]";   //value is last here but in place of the "right" register
    format[sub]         ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[lt]          ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[lor]         ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[shra]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[ishra]       ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[0,5] [5]value->[5,5]";   //value is last here but in place of the "right" register
    format[addc]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[ult]         ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[land]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[subc]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
    format[lxor]        ="[9]opcode->[23,9] [5]rID->[10,5] [5]rID->[5,5] [5]rID->[0,5]";
   
    
}

//------------------------------------------------------------------------------
private Opcodes(){
}

//------------------------------------------------------------------------------
public static String[] getFormat(int _opcode, int _variant){
    try{
        if(format[_opcode].length() == 0){
            return new String[0];
        }else{
            String[] _variants = format[_opcode].split("\\|");
            return _variants[_variant].split(" ");
        }
    }catch(NullPointerException _npe){
        throw new RuntimeException("Unknown opcode: " + Integer.toHexString(_opcode));
    }
}

//------------------------------------------------------------------------------
public static String[] getFormatByName(String _opcode){
    return getFormat(getOpcodeByName(_opcode) & 0x1ff, getOpcodeVariantByName(_opcode));
}

//------------------------------------------------------------------------------
public static String getNameByOpcode(short _opcode){
String _opcodeMenomnic;
//--
    _opcodeMenomnic = opCodes[_opcode];
    if(_opcodeMenomnic == null){
        throw new RuntimeException("Unknown opcode: " + _opcode);
    }

    return _opcodeMenomnic.indexOf('|') >= 0 ? _opcodeMenomnic.split("\\|")[0] : _opcodeMenomnic;
}

//------------------------------------------------------------------------------
public static short getOpcodeByName(String _name){
    for(int i=0; i<opCodes.length; i++){
        if(opCodes[i] == null){
            continue;
        }
        String[] _names = opCodes[i].split("\\|", -1);
        for(int j=0; j<_names.length; j++){
            if(_name.equals(_names[j])){
                return (short)i;
            }
        }
    }

    throw new RuntimeException("Unknown opcode: " + _name);
}

//------------------------------------------------------------------------------
private static short getOpcodeVariantByName(String _name){
    for(int i=0; i<opCodes.length; i++){
        if(opCodes[i] == null){
            continue;
        }
        String[] _names = opCodes[i].split("\\|", -1);
        for(int j=0; j<_names.length; j++){
            if(_name.equals(_names[j])){
                return (short)j;
            }
        }
    }

    throw new RuntimeException("Unknown opcode: " + _name);
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Opcodes.java,java $
