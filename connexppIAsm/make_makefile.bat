cd auto
cbp2make -in ../connexppAsm.cbp -out ../makefile --all-os -targets "Debug"
cbp2make -in ../connexppAsm.cbp -out ../makefiler --all-os -targets "Release"
PAUSE