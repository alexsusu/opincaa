/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.hardware;


/**
 *
 * @author rhobincu
 */
public class MachineConstants {

    public static final int BOOT_ADDRESS = 0;
    public static final int THREAD_COUNT = 4;
    public static final int ARRAY_LENGTH = 128;
    public static final int STACK_SIZE = 4 * 1024;
    public static final int TEXT_AREA = 0x1000;
    public static final int IRQ_HANDLER_ADDR = 0x1000;
    public static final int HW_REG_LINK = 10;
    public static final int HW_IRQ_HANDLER = 0x1000;
    public static final int SW_REG_LINK = 9;
    public static final int SW_IRQ_HANDLER = 0x2000;
    public static final int THREAD_LIST_ADDRESS = 0x1040000 + 5000;

}
