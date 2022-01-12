CFLAGS  := -O3 -Wall -Werror -Wno-unused-result -DVALUE_SIZE=${VALUE_SIZE} -DPUT_PERCENT=${PUT_PERCENT} -DNUM_KEYS=${NUM_KEYS} -DNUM_KEYS_=${NUM_KEYS_} -DZIPF=${ZIPF}
LD      := gcc
LDFLAGS := ${LDFLAGS} -lrdmacm -libverbs -lrt -lpthread

main: common.o conn.o main.o
	${LD} -o $@ $^ ${LDFLAGS}

PHONY: clean
clean:
	rm -f *.o main
