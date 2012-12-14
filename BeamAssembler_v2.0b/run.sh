LIBPATH=`pwd`/../simulator/:`pwd`/../fpgaInterface/
echo $LIBPATH
java -Djava.library.path=$LIBPATH -Xms1024m -Xmx2048m -jar dist/BeamAssembler_v1.0.jar
