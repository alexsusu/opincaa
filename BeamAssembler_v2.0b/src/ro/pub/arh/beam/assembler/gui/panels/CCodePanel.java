/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * AsmCodePanel.java
 *
 * Created on 03.07.2009, 16:28:23
 */

package ro.pub.arh.beam.assembler.gui.panels;

import java.io.*;
import ro.pub.arh.beam.assembler.gui.AssemblerGui;
import ro.pub.arh.beam.utils.gui.HighlightedTextArea;

/**
 *
 * @author Ares
 */
public class CCodePanel extends CodePanel{

    /** Creates new form AsmCodePanel */
    public CCodePanel(AssemblerGui _parentFrame) {
        super(_parentFrame);
        extension = ".c,.cpp";
        initFileChooser();
    }

    public CCodePanel(AssemblerGui _parentFrame, String _fileName) throws IOException{
        super(_parentFrame, _fileName);
        extension = ".c,.cpp";
        updateHighlight();
    }

    @Override
    public void updateHighlight() {
        HighlightedTextArea _codeArea = (HighlightedTextArea)codeArea;
        _codeArea.removeHighlights();
        if(!highlight){
            return;
        }
//        _codeArea.highlight("//.*+", new Color(200, 200, 200));
//        _codeArea.highlight("[rR]\\d{1,2}", new Color(200, 200, 255));
//        for(int i=0; i<255; i++){
//            try{
//                String _mnemonic = Opcodes.getNameByOpcode((byte)i);
//                _codeArea.highlight(_mnemonic, new Color(255, 200, 200));
//            }catch(Exception _e){
//
//            }
//        }
//        _codeArea.highlight("0x\\p{XDigit}++", new Color(255, 200, 255));
//        _codeArea.highlight("\\d++", new Color(255, 200, 255));
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(this);
        this.setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 623, Short.MAX_VALUE)
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 419, Short.MAX_VALUE)
        );
    }// </editor-fold>//GEN-END:initComponents


    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables

}
