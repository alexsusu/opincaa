/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

/**
 *
 * @author Echo
 */
public class RTC implements Peripheral{

    private int mapAddress;
    private static final int REGISTER_COUNT = 3;
    private int[] registers;

    private long insns;

public RTC(int _address){
    mapAddress = _address;
    registers = new int[REGISTER_COUNT];
}

public void count(){
    if((registers[0] & 1) != 0){
        insns = (insns + 1) % 100000;
        registers[2] += insns == 99999 ? 1 : 0;

        if(registers[2] == registers[1] && (registers[0] & 2) != 0){
            registers[2] = 0;
            registers[0] |= 8;
        }
    }
}

public int read(int _address) {
    int _regOffset = (_address - mapAddress) >>> 2;
    return registers[_regOffset];
}

public void write(int _address, int _data) {
    int _regOffset = (_address - mapAddress) >>> 2;
    registers[_regOffset] = _data;
    if(_regOffset == 0 && (_data & 4) != 0){
        registers[0] &= ~8;
    }
}

public boolean isMapped(int _address) {
    return _address >= mapAddress && _address < mapAddress + REGISTER_COUNT * 4;
}

public boolean hasInterruptCapabilities() {
    return true;
}

public boolean interruptActive() {
    return (registers[0] & 8) != 0;
}

}
