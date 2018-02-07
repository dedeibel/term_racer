all: term_racer term_racer_simple term_editor thread_racer thread_editor

CFLAGS += -Wall

term_editor: term_editor.c

term_racer: term_racer.c

term_racer_simple: term_racer_simple.c

thread_editor: LDFLAGS=-lpthread
thread_editor: thread_editor.c

thread_racer: LDFLAGS=-lpthread
thread_racer: thread_racer.c

clean:
	rm -f term_racer
	rm -f term_racer_simple
	rm -f term_editor
	rm -f thread_racer
	rm -f thread_editor
