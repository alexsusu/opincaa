/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

/**
 *
 * @author rhobincu
 */
public interface InputOutputPeripheral extends Peripheral{
    public int externalRead();
    public void externalWrite(int _data);
    public int externalAvailable();
}
