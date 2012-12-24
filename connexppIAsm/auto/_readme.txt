For a deployment to occur we must do:

a) call cbp2make: 	cbp2make -in connexppAsm.cbp -out makefile --all-os -targets "linDebug_x86"
b) copy source files to arm machine (done via auto_ftp...) including makefile
c) ssh to arm machine 
d) call make:	make -f makefile.unix all
e) call resulting binary	./bin/Debug/connexppAsm
f) quit ssh