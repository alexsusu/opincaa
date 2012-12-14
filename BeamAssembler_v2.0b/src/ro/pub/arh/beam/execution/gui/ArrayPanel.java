/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * ArrayPanel.java
 *
 * Created on Aug 19, 2010, 6:45:08 PM
 */

package ro.pub.arh.beam.execution.gui;

import javax.swing.JTable;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import ro.pub.arh.beam.hardware.MachineConstants;
import ro.pub.arh.beam.hardware.emulator.core.array.ArrayRegisterFile;
import ro.pub.arh.beam.hardware.emulator.core.array.Line;
import ro.pub.arh.beam.hardware.emulator.core.array.ArrayCore;
import ro.pub.arh.beam.hardware.emulator.core.BeamRegisterFile;

/**
 *
 * @author rhobincu
 */
public class ArrayPanel extends javax.swing.JPanel {

    /** Creates new form ArrayPanel */
    public ArrayPanel() {
        initComponents();
        initTables();
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jSplitPane1 = new javax.swing.JSplitPane();
        jScrollPane1 = new javax.swing.JScrollPane();
        jTable1 = new javax.swing.JTable();
        jSplitPane2 = new javax.swing.JSplitPane();
        jScrollPane2 = new javax.swing.JScrollPane();
        jTable2 = new javax.swing.JTable();
        jScrollPane3 = new javax.swing.JScrollPane();
        jTable3 = new javax.swing.JTable();

        jSplitPane1.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);

        jScrollPane1.setHorizontalScrollBarPolicy(javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
        jScrollPane1.setVerticalScrollBarPolicy(javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);

        jTable1.setModel(new ArrayTableModel(ArrayRegisterFile.REGISTER_COUNT,
            MachineConstants.ARRAY_LENGTH + 1)
    );
    jScrollPane1.setViewportView(jTable1);

    jSplitPane1.setLeftComponent(jScrollPane1);

    jSplitPane2.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);

    jScrollPane2.setHorizontalScrollBarPolicy(javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
    jScrollPane2.setVerticalScrollBarPolicy(javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);

    jTable2.setModel(new ArrayTableModel(1, MachineConstants.ARRAY_LENGTH + 1)
    );
    jScrollPane2.setViewportView(jTable2);

    jSplitPane2.setLeftComponent(jScrollPane2);

    jScrollPane3.setHorizontalScrollBarPolicy(javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS);
    jScrollPane3.setVerticalScrollBarPolicy(javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);

    jTable3.setModel(new ArrayTableModel(ArrayCore.ARRAY_MEMORY_SIZE, MachineConstants.ARRAY_LENGTH + 1)
    );
    jScrollPane3.setViewportView(jTable3);

    jSplitPane2.setRightComponent(jScrollPane3);

    jSplitPane1.setRightComponent(jSplitPane2);

    javax.swing.GroupLayout layout = new javax.swing.GroupLayout(this);
    this.setLayout(layout);
    layout.setHorizontalGroup(
        layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
        .addGroup(layout.createSequentialGroup()
            .addContainerGap()
            .addComponent(jSplitPane1, javax.swing.GroupLayout.DEFAULT_SIZE, 737, Short.MAX_VALUE)
            .addContainerGap())
    );
    layout.setVerticalGroup(
        layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
        .addGroup(javax.swing.GroupLayout.Alignment.TRAILING, layout.createSequentialGroup()
            .addContainerGap()
            .addComponent(jSplitPane1, javax.swing.GroupLayout.DEFAULT_SIZE, 609, Short.MAX_VALUE)
            .addContainerGap())
    );
    }// </editor-fold>//GEN-END:initComponents


    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JScrollPane jScrollPane1;
    private javax.swing.JScrollPane jScrollPane2;
    private javax.swing.JScrollPane jScrollPane3;
    private javax.swing.JSplitPane jSplitPane1;
    private javax.swing.JSplitPane jSplitPane2;
    private javax.swing.JTable jTable1;
    private javax.swing.JTable jTable2;
    private javax.swing.JTable jTable3;
    // End of variables declaration//GEN-END:variables

private void initTables() {
    DefaultTableModel _tableModel = (DefaultTableModel) jTable1.getModel();
    for(int i=0; i<_tableModel.getRowCount(); i++){
        _tableModel.setValueAt("R" + (BeamRegisterFile.REGISTER_COUNT + i), i, 0);
    }

    _tableModel = (DefaultTableModel) jTable2.getModel();
    _tableModel.setValueAt("Flags", 0, 0);

    _tableModel = (DefaultTableModel) jTable3.getModel();
    for(int i=0; i<_tableModel.getRowCount(); i++){
        _tableModel.setValueAt(i + "", i, 0);
    }

    jScrollPane1.setHorizontalScrollBar(jScrollPane3.getHorizontalScrollBar());
    jScrollPane2.setHorizontalScrollBar(jScrollPane3.getHorizontalScrollBar());
    jScrollPane3.setHorizontalScrollBar(jScrollPane3.getHorizontalScrollBar());
    jSplitPane1.setDividerLocation(0.7);
    jSplitPane2.setDividerLocation(0.2);
    jTable1.getTableHeader().setReorderingAllowed(false);
    jTable2.getTableHeader().setReorderingAllowed(false);
    jTable3.getTableHeader().setReorderingAllowed(false);
}

public void updateRegister(int _register, Line _data){
    DefaultTableModel _tableModel = (DefaultTableModel) jTable1.getModel();
    for(int i=1; i<_tableModel.getColumnCount(); i++){
        String _value = Integer.toHexString(_data.getCell(i - 1));
        while(_value.length() < 4){
            _value = "0" + _value;
        }
        _tableModel.setValueAt("0x" + _value.substring(_value.length() - 4), _register, i);
    }
}

public void updateMemory(int _address, Line _data){
    DefaultTableModel _tableModel = (DefaultTableModel) jTable3.getModel();
    for(int i=1; i<_tableModel.getColumnCount(); i++){
        String _value = Integer.toHexString(_data.getCell(i - 1));
        while(_value.length() < 4){
            _value = "0" + _value;
        }
        _tableModel.setValueAt("0x" + _value.substring(_value.length() - 4), _address, i);
    }
}

public void updateFlags(Line _data){
    DefaultTableModel _tableModel = (DefaultTableModel) jTable2.getModel();
    for(int i=1; i<_tableModel.getColumnCount(); i++){
        String _value = Integer.toHexString(_data.getCell(i - 1));
        while(_value.length() < 4){
            _value = "0" + _value;
        }
        _tableModel.setValueAt("0x" + _value.substring(_value.length() - 4), 0, i);
    }
}

public void reset() {
    jTable1.setModel(new ArrayTableModel(ArrayRegisterFile.REGISTER_COUNT, MachineConstants.ARRAY_LENGTH + 1));
    jTable2.setModel(new ArrayTableModel(1, MachineConstants.ARRAY_LENGTH + 1));
    jTable3.setModel(new ArrayTableModel(ArrayCore.ARRAY_MEMORY_SIZE, MachineConstants.ARRAY_LENGTH + 1));
    initTables();
    setColumnSizes(jTable1);
    setColumnSizes(jTable2);
    setColumnSizes(jTable3);
}

private void setColumnSizes(JTable _jTable) {
    _jTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
    for(int i=0; i<_jTable.getColumnCount(); i++){
        TableColumn _column = _jTable.getColumnModel().getColumn(i);
        _column.setResizable(false);

        _column.setPreferredWidth(60);
        _column.setMinWidth(60);
        _column.setMaxWidth(60);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class ArrayTableModel extends DefaultTableModel{

    public ArrayTableModel(int _rows, int _columns){
        super(_rows, _columns);
        nullTable();
        setColumnIdentifiers(getCellIdentifiers());
    }

    private void nullTable(){
        for(int i=0; i<getRowCount(); i++){
            for(int j=1; j<getColumnCount(); j++){
                setValueAt("0x0000", i, j);
            }
        }
    }

    private String[] getCellIdentifiers(){
        String[] _idents = new String[MachineConstants.ARRAY_LENGTH + 1];
        _idents[0] = "";
        for(int i=1; i<_idents.length; i++){
            _idents[i] = String.valueOf(i - 1);
        }
        return _idents;
    }

        @Override
    public Class getColumnClass(int columnIndex) {
        return String.class;
    }

        @Override
    public boolean isCellEditable(int rowIndex, int columnIndex) {
        return false;
    }
}
//------------------------------------------------------------------------------

}
