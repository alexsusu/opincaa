/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package ro.pub.arh.beam.structs;

/**
 *
 * @author rhobincu
 */
public class MemoryLocationsList {

    private Node<AddressDataPair> firstNode;

    private int nodeCount;

public MemoryLocationsList(){
    firstNode = new Node(new AddressDataPair(-1, 0));
    nodeCount = 1;
}

public void add(AddressDataPair _pair){

    //get the address closest but less than the new address
    Node<AddressDataPair> _node = lookupPreviousAddress(_pair.address);
    if(_node.getData().address == _pair.address){
        _node.setData(_pair);
        return;
    }

    //add the new node in the list after node
    Node<AddressDataPair> _newNode = new Node(_pair);
    _newNode.setPreviousNode(_node);
    _newNode.setNextNode(_node.getNextNode());

    _node.setNextNode(_newNode);
    _node.getNextNode().setPreviousNode(_newNode);
}

private Node<AddressDataPair> lookupPreviousAddress(int _address) {
    Node<AddressDataPair> _node = firstNode;
    Node<AddressDataPair> _nextNode = _node.getNextNode();
    
    while(true){
        if(_nextNode == null || _nextNode.getData().address > _address){
            return _node;
        }
        _node = _node.getNextNode();
        _nextNode = _nextNode.getNextNode();
    }
}

public AddressDataPair get(int _address) {
    Node<AddressDataPair> _node = lookupPreviousAddress(_address);
    if(_node.getData().address == _address){
        return _node.getData();
    }
    return new AddressDataPair(_address, 0);
}

}
