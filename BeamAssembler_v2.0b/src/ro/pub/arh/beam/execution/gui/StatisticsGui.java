/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * StatisticsGui.java
 *
 * Created on 19.11.2009, 19:15:16
 */

package ro.pub.arh.beam.execution.gui;

import javax.swing.table.DefaultTableModel;
import ro.pub.arh.beam.hardware.Opcodes;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.core.*;

/**
 *
 * @author Ares
 */
public class StatisticsGui extends javax.swing.JFrame {

    /** Creates new form StatisticsGui */
    public StatisticsGui(Statistics[] _statistics) {
        if(_statistics == null){
            return;
        }
        initComponents();
        initData(_statistics);
        setVisible(true);
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jScrollPane1 = new javax.swing.JScrollPane();
        jTable1 = new javax.swing.JTable();
        jScrollPane2 = new javax.swing.JScrollPane();
        jTable2 = new javax.swing.JTable();

        setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        setTitle("Statistics for current run");
        setAlwaysOnTop(true);

        jTable1.setModel(new javax.swing.table.DefaultTableModel(
            new Object [][] {

            },
            new String [] {
                "Instruction", "Thread 0", "Thread 1", "Thread 2", "Thread 3"
            }
        ) {
            Class[] types = new Class [] {
                java.lang.String.class, java.lang.Long.class, java.lang.Long.class, java.lang.Long.class, java.lang.Long.class
            };
            boolean[] canEdit = new boolean [] {
                false, false, false, false, false
            };

            public Class getColumnClass(int columnIndex) {
                return types [columnIndex];
            }

            public boolean isCellEditable(int rowIndex, int columnIndex) {
                return canEdit [columnIndex];
            }
        });
        jScrollPane1.setViewportView(jTable1);

        jTable2.setModel(new javax.swing.table.DefaultTableModel(
            new Object [][] {
                {"Jump outside buffer range", new Integer(0), new Integer(0), new Integer(0), new Integer(0)}
            },
            new String [] {
                "Stat", "Thread 0", "Thread 1", "Thread 2", "Thread 3"
            }
        ) {
            Class[] types = new Class [] {
                java.lang.String.class, java.lang.Integer.class, java.lang.Integer.class, java.lang.Integer.class, java.lang.Integer.class
            };
            boolean[] canEdit = new boolean [] {
                false, false, false, false, false
            };

            public Class getColumnClass(int columnIndex) {
                return types [columnIndex];
            }

            public boolean isCellEditable(int rowIndex, int columnIndex) {
                return canEdit [columnIndex];
            }
        });
        jScrollPane2.setViewportView(jTable2);

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addComponent(jScrollPane2, javax.swing.GroupLayout.DEFAULT_SIZE, 626, Short.MAX_VALUE)
            .addComponent(jScrollPane1, javax.swing.GroupLayout.DEFAULT_SIZE, 626, Short.MAX_VALUE)
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(javax.swing.GroupLayout.Alignment.TRAILING, layout.createSequentialGroup()
                .addComponent(jScrollPane1, javax.swing.GroupLayout.DEFAULT_SIZE, 160, Short.MAX_VALUE)
                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                .addComponent(jScrollPane2, javax.swing.GroupLayout.DEFAULT_SIZE, 173, Short.MAX_VALUE))
        );

        pack();
    }// </editor-fold>//GEN-END:initComponents

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JScrollPane jScrollPane1;
    private javax.swing.JScrollPane jScrollPane2;
    private javax.swing.JTable jTable1;
    private javax.swing.JTable jTable2;
    // End of variables declaration//GEN-END:variables

    private void initData(Statistics[] _statistics) {
        DefaultTableModel _model = (DefaultTableModel)jTable1.getModel();
        String _name = "";
        for(short j=0; j<Short.MAX_VALUE; j++){
            boolean _appearedOnce = false;
            Object[] _rowData = new Object[MachineConstants.THREAD_COUNT + 1];
            for(int i=0; i<MachineConstants.THREAD_COUNT; i++){
                _rowData[i + 1] = new Long(_statistics[i].getOccurencesOfInstruction(j));
                if(_statistics[i].getOccurencesOfInstruction(j) != 0){
                    _appearedOnce = true;
                }
            }
            if(_appearedOnce){
                _rowData[0] = Opcodes.getNameByOpcode(j);
                _model.addRow(_rowData);
            }
        }

        _model = (DefaultTableModel)jTable2.getModel();
        for(int i=0; i<MachineConstants.THREAD_COUNT; i++){
            _model.setValueAt(_statistics[i].getJumpOutsideBufferRange(), 0, 1 + i);
        }
    }

}
