CC=cc
CFLAGS=-Wall -Werror -std=c89 -pedantic
LDFLAGS=

all:V: bincat binstrip binwrite headerdump

%: %.o
	$CC -o $stem $stem.c $LDFLAGS

%.o: %.c
	$CC $CFLAGS -c $stem.c
