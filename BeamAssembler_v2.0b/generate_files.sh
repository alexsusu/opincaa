DIR="$( cd "$( dirname "$0" )" && pwd )"
java -cp $DIR/dist/BeamAssembler_v1.0.jar ro.pub.arh.beam.linker.Linker "$@"
