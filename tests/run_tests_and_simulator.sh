killall -9 simulator
killall -9 simulator
rm distributionFIFO readFIFO reductionFIFO writeFIFO

source ./test_harness.sh connex16-hm-generic

killall -9 simulator
