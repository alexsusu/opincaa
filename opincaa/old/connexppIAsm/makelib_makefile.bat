cd auto
cbp2make -in ../libopincaa.cbp -out ../makelibfile --all-os -targets "DebugLib"
cbp2make -in ../libopincaa.cbp -out ../makelibrfile --all-os -targets "ReleaseLib"
PAUSE