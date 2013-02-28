cd auto
cbp2make -in ../connexppAsm.cbp -out ../makefile --all-os -targets "DebugArm"
cbp2make -in ../connexppAsm.cbp -out ../makefiler --all-os -targets "ReleaseArm"
PAUSE