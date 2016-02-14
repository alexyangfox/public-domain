
all:	hlaasm.masm hlaasm.gas hlaasm.gasx \
		hlabe.linux.gas hlabe.freeBSD.gas hlabe.gasx hlabe.masm 

hlaasm.masm:	
	hla -sourcemode -s -masm -sourcemode hlaasm.hla
	copy hlaasm.asm ..\hlaasm.masm
	ml -c -coff -Cp hlaasm.asm
	copy hlaasm.obj ..\hlaasm.obj
	
	
hlaasm.gas:
	hla -sourcemode -s -gas -sourcemode -dunix hlaasm.hla
	copy hlaasm.asm ..\hlaasm.gas
	
	
hlaasm.gasx:
	hla -macos -sourcemode -s -gasx -sourcemode -dmacosx hlaasm.hla
	copy hlaasm.asm ..\hlaasm.gasx
	
	
hlabe.masm: hlabe.hla hlabe.hhf
	hla -sourcemode -s -masm -sourcemode -DWINDOWS hlabe
	copy hlabe.asm ..\hlabe.masm
	ml -c -coff -Cp hlabe.asm
	copy hlabe.obj ..\hlabe.obj
	
	
hlabe.linux.gas: hlabe.hla hlabe.hhf
	hla -sourcemode -s -gas -sourcemode -DUNIX -linux hlabe
	copy hlabe.asm ..\hlabe.linux.gas
	
hlabe.freeBSD.gas: hlabe.hla hlabe.hhf
	hla -sourcemode -s -gas -sourcemode -DUNIX -freebsd hlabe
	copy hlabe.asm ..\hlabe.freeBSD.gas
	
	
hlabe.gasx: hlabe.hla hlabe.hhf
	hla -sourcemode -s -gasx -sourcemode -DUNIX -macos -dmacosx hlabe
	copy hlabe.asm ..\hlabe.gasx
	
	
clean:
	delete *.obj
	delete *.asm
	delete *.bak
	delete *.link
	delete *.inc
	delete *.masm
	delete *.gas
	delete *.gasx

