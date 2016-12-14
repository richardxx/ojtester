CC=gcc #-4.3.3
OPTIMIZE=
CFLAGS= #-fdump-func-info -g #${OPTIMIZE} -finstrument-functions
DUMP_FLAGS= #-fdump-ipa-cgraph
LINKLIB=
MACROS= -DDEBUG 
HEADERS=consts.h judge.h runtime.h type_def.h file.h libsys.h libprocs.h
SOURCES=libprocs.c file.c judge.c libsys.c main.c runtime.c #instrument.c


all: tester
tester: ${HEADERS} ${SOURCES}
	${CC} ${CFLAGS} ${LINKLIB} ${MACROS} ${SOURCES} -o tester 


install: tester
	@if [ -d "${HOME}/bin" ]; then \
		cp "tester" "${HOME}/bin"; \
	else \
		cp "tester" "/usr/local/bin"; \
	fi


clean:
	@rm -f tester
	@rm -f *.o
	@for s in *.{rel,dot,expand,o}; do \
		rm -f $$s; \
	done

