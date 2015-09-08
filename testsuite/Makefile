test:
	rm -f tmp.asm
	for i in *.asm ; do \
	  echo "==== $$i" && \
	  cp $$i tmp.asm && \
	  ../asm.pl tmp.asm && \
	  ../larum.exe tmp.img ; \
	done
	rm tmp.*
