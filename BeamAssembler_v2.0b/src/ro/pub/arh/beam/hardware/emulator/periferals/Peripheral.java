/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware.emulator.periferals;

/**
 *
 * @author rhobincu
 */
public interface Peripheral {

    public int read(int _address);
    public void write(int _address, int _data);
    public boolean isMapped(int _address);
    public boolean hasInterruptCapabilities();

    public boolean interruptActive();
}
