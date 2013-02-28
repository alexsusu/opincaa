cd auto
cbp2make -in ../libopincaa.cbp -out ../makelibfile --all-os -targets "DebugLibArm" 
cbp2make -in ../libopincaa.cbp -out ../makelibfiler --all-os -targets "ReleaseLibArm"
PAUSE