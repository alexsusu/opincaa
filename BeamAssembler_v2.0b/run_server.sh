killall -9 java
LIBPATH=`pwd`/../simulator/:`pwd`/../fpgaInterface/
java -Djava.library.path=$LIBPATH -cp dist/BeamAssembler_v1.0.jar ro.pub.arh.beam.hardware.fpga.network.FPGAConnectionServer
