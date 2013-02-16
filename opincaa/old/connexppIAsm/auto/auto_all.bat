cbp2make -in ../connexppAsm.cbp -out ../makefile --all-os -targets "linDebug_x86"
cd ..
ftp -i -A -s:auto\auto_ftp_cmds.txt 192.168.1.6
cd auto

putty -ssh -l root -P 22 192.168.1.6 -pw cc -m auto_putty.txt