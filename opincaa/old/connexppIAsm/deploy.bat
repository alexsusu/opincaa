cd auto
cbp2make -in ../connexppAsm.cbp -out ../makefile --all-os -targets "Debug"

putty -ssh -l root -P 22 141.85.94.100  -pw cc -m auto_putty_remove.txt

cd..
ftp -i -A -s:auto\auto_ftp_cmds.txt 141.85.94.100 
cd auto

putty -ssh -l root -P 22 141.85.94.100  -pw cc -m auto_putty.txt
cd..
PAUSE