-include config.mk

### Options ###
#CXX=icc
#CXX=clang
#CXX=/home/ipandis/apps/bin/gcc483/g++
CXX=g++
#CXX=g++491

DEBUG ?= 0
NDEBUG ?= 0

# run with 'MASSTREE=0' to turn off masstree
MASSTREE ?= 1

###############

DEBUG_S=$(strip $(DEBUG))
NDEBUG_S=$(strip $(NDEBUG))
MASSTREE_S=$(strip $(MASSTREE))
MASSTREE_CONFIG:=--enable-max-key-len=1024
MASSTREE_LDFLAGS:=

SSI_S=$(strip $(SSI))
SI_SSN_S=$(strip $(SI_SSN))
RC_SSN_S=$(strip $(RC_SSN))

ifeq ($(DEBUG_S),1)
	OSUFFIX_D=.debug
	MASSTREE_CONFIG+=--enable-assertions
else
	MASSTREE_CONFIG+=--disable-assertions
endif
ifeq ($(NDEBUG_S),0)
	OSUFFIX_S=.check
	MASSTREE_CONFIG+=--enable-invariants --enable-preconditions
else
	MASSTREE_CONFIG+=--disable-invariants --disable-preconditions
endif
OSUFFIX=$(OSUFFIX_D)$(OSUFFIX_S)$(OSUFFIX_E)

O := out-perf$(OSUFFIX)

CXXFLAGS := -Wall -std=c++0x
CXXFLAGS += -MD -Ithird-party/sparsehash/src

ifeq ($(SSI_S),1)
	CXXFLAGS += -DSSI
else ifeq ($(RC_SSN_S),1)
	CXXFLAGS += -DSSN -DRC -DEARLY_SSN_CHECK
else ifeq ($(SI_SSN_S),1)
	CXXFLAGS += -DSSN -DEARLY_SSN_CHECK
endif

ifeq ($(DEBUG_S),1)
        CXXFLAGS +=  -g -gdwarf-2 -fno-omit-frame-pointer -DDEBUG #-fsanitize=address
else
        CXXFLAGS += -march=native -O2 -funroll-loops -fno-omit-frame-pointer
endif
ifeq ($(NDEBUG_S),1)
	CXXFLAGS += -DNDEBUG
endif
ifeq ($(MASSTREE_S),1)
	CXXFLAGS += -DMASSTREE -include masstree/config.h
	OBJDEP += masstree/config.h
	O := $(O).masstree
else
	O := $(O).silotree
endif

TOP     := $(shell echo $${PWD-`pwd`})
LDFLAGS := -lpthread -lnuma -lrt
#ifeq ($(DEBUG_S),1)
#LDFLAGS += -lasan
#endif

MASSTREE_CONFIG+=--with-malloc=malloc

ifneq ($(strip $(CUSTOM_LDPATH)), )
        LDFLAGS+=$(CUSTOM_LDPATH)
endif

SRCFILES = base_txn_btree.cc \
	object.cc \
	thread.cc \
	tuple.cc \
	txn.cc \
	varint.cc

DBCORE_SRCFILES = dbcore/burt-hash.cpp \
	dbcore/sm-alloc.cpp \
	dbcore/sm-chkpt.cpp \
	dbcore/sm-config.cpp \
	dbcore/sm-log.cpp \
	dbcore/sm-file.cpp \
	dbcore/sm-tx-log.cpp \
	dbcore/sm-log-alloc.cpp \
	dbcore/sm-log-recover.cpp \
	dbcore/sm-log-offset.cpp \
	dbcore/sm-log-file.cpp \
	dbcore/sm-log-recover-impl.cpp \
	dbcore/sm-oid.cpp \
	dbcore/sm-oid-alloc-impl.cpp \
	dbcore/sm-exceptions.cpp \
	dbcore/sm-common.cpp \
	dbcore/sm-thread.cpp \
	dbcore/window-buffer.cpp \
	dbcore/rcu-slist.cpp \
	dbcore/rcu.cpp \
	dbcore/epoch.cpp \
	dbcore/adler.cpp \
	dbcore/w_rand.cpp \
	dbcore/size-encode.cpp \
	dbcore/xid.cpp		\
	dbcore/serial.cpp	\
	dbcore/dynarray.cpp	\
	dbcore/mcs_lock.cpp	

ifeq ($(MASSTREE_S),1)
MASSTREE_SRCFILES = masstree/compiler.cc \
	masstree/str.cc \
	masstree/string.cc \
	masstree/straccum.cc \
	masstree/json.cc
endif

OBJFILES := $(patsubst %.cc, $(O)/%.o, $(SRCFILES))

MASSTREE_OBJFILES := $(patsubst masstree/%.cc, $(O)/%.o, $(MASSTREE_SRCFILES))
DBCORE_OBJFILES := $(patsubst dbcore/%.cpp, $(O)/dbcore/%.o, $(DBCORE_SRCFILES))

dbcore/burt-hash.cpp:
	@python2 dbcore/burt-hash.py > dbcore/burt-hash.cpp

BENCH_CXXFLAGS := $(CXXFLAGS)
BENCH_LDFLAGS := $(LDFLAGS)

BENCH_SRCFILES = \
	benchmarks/ndb_wrapper.cc \
	benchmarks/bench.cc \
	benchmarks/tpce.cc	\
	benchmarks/tpcc.cc  \
	benchmarks/ycsb.cc

EGEN_SRCFILES = \
   benchmarks/egen/DateTime.cpp \
   benchmarks/egen/error.cpp \
   benchmarks/egen/Random.cpp \
   benchmarks/egen/Money.cpp \
   benchmarks/egen/EGenVersion.cpp \
   benchmarks/egen/locking.cpp \
   benchmarks/egen/threading.cpp \
   benchmarks/egen/BaseLogger.cpp \
   benchmarks/egen/EGenLogFormatterTab.cpp \
   benchmarks/egen/EGenLoader.cpp \
   benchmarks/egen/MEE.cpp \
   benchmarks/egen/MEEPriceBoard.cpp \
   benchmarks/egen/MEESecurity.cpp \
   benchmarks/egen/MEETickerTape.cpp \
   benchmarks/egen/MEETradingFloor.cpp \
   benchmarks/egen/WheelTime.cpp \
   benchmarks/egen/AddressTable.cpp \
   benchmarks/egen/CustomerSelection.cpp \
   benchmarks/egen/CustomerTable.cpp \
   benchmarks/egen/InputFlatFilesStructure.cpp \
   benchmarks/egen/Person.cpp \
   benchmarks/egen/ReadRowFunctions.cpp \
   benchmarks/egen/TradeGen.cpp \
   benchmarks/egen/FlatFileLoader.cpp \
   benchmarks/egen/CE.cpp \
   benchmarks/egen/CETxnInputGenerator.cpp \
   benchmarks/egen/CETxnMixGenerator.cpp \
   benchmarks/egen/DM.cpp \
   benchmarks/egen/EGenGenerateAndLoad.cpp \
   benchmarks/egen/EGenValidate.cpp \
   benchmarks/egen/strutil.cpp \
   benchmarks/egen/progressmeter.cpp \
   benchmarks/egen/progressmeterinterface.cpp \
   benchmarks/egen/bucketsimulator.cpp

BENCH_OBJFILES := $(patsubst %.cc, $(O)/%.o, $(BENCH_SRCFILES))
EGEN_OBJFILES := $(patsubst %.cpp, $(O)/%.o, $(EGEN_SRCFILES))

NEWBENCH_SRCFILES = new-benchmarks/bench.cc \
	new-benchmarks/tpcc.cc

NEWBENCH_OBJFILES := $(patsubst %.cc, $(O)/%.o, $(NEWBENCH_SRCFILES))

all: $(O)/test

$(O)/benchmarks/%.o: benchmarks/%.cc $(O)/buildstamp $(O)/buildstamp.bench $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(BENCH_CXXFLAGS) -c $< -o $@

$(O)/benchmarks/masstree/%.o: benchmarks/masstree/%.cc $(O)/buildstamp $(O)/buildstamp.bench $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(BENCH_CXXFLAGS) -c $< -o $@

$(EGEN_OBJFILES) : $(O)/benchmarks/egen/%.o: benchmarks/egen/%.cpp $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(O)/new-benchmarks/%.o: new-benchmarks/%.cc $(O)/buildstamp $(O)/buildstamp.bench $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(O)/%.o: %.cc $(O)/buildstamp $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MASSTREE_OBJFILES) : $(O)/%.o: masstree/%.cc masstree/config.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -include masstree/config.h -c $< -o $@

$(DBCORE_OBJFILES) : $(O)/dbcore/%.o: dbcore/%.cpp $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: test
test: $(O)/test

$(O)/test: $(O)/test.o $(OBJFILES) $(DBCORE_OBJFILES) $(MASSTREE_OBJFILES)
	$(CXX) -o $(O)/test $^ $(LDFLAGS)

masstree/config.h: $(O)/buildstamp.masstree masstree/configure masstree/config.h.in
	rm -f $@
	cd masstree; LDFLAGS=$(MASSTREE_LDFLAGS) ./configure $(MASSTREE_CONFIG)
	if test -f $@; then touch $@; fi

masstree/configure masstree/config.h.in: masstree/configure.ac
	cd masstree && autoreconf -i && touch configure config.h.in

.PHONY: dbtest
dbtest: $(O)/benchmarks/dbtest

$(O)/benchmarks/dbtest: $(O)/benchmarks/dbtest.o $(OBJFILES) $(DBCORE_OBJFILES) $(MASSTREE_OBJFILES) $(BENCH_OBJFILES) $(EGEN_OBJFILES)
	$(CXX) -o $(O)/benchmarks/dbtest $^ $(BENCH_LDFLAGS)

.PHONY: kvtest
kvtest: $(O)/benchmarks/masstree/kvtest

$(O)/benchmarks/masstree/kvtest: $(O)/benchmarks/masstree/kvtest.o $(OBJFILES) $(DBCORE_OBJFILES) $(BENCH_OBJFILES)
	$(CXX) -o $(O)/benchmarks/masstree/kvtest $^ $(BENCH_LDFLAGS)

.PHONY: newdbtest
newdbtest: $(O)/new-benchmarks/dbtest

$(O)/new-benchmarks/dbtest: $(O)/new-benchmarks/dbtest.o $(OBJFILES) $(DBCORE_OBJFILES) $(MASSTREE_OBJFILES) $(NEWBENCH_OBJFILES)
	$(CXX) -o $(O)/new-benchmarks/dbtest $^ $(LDFLAGS)

DEPFILES := $(wildcard $(O)/*.d $(O)/*/*.d $(O)/*/*/*.d masstree/_masstree_config.d)
ifneq ($(DEPFILES),)
-include $(DEPFILES)
endif

ifeq ($(wildcard masstree/GNUmakefile.in),)
#INSTALL_MASSTREE := $(shell git submodule init; git submodule update)
endif

ifeq ($(MASSTREE_S),1)
#UPDATE_MASSTREE := $(shell cd ./`git rev-parse --show-cdup` && cur=`git submodule status --cached masstree | head -c 41 | tail -c +2` && if test -z `cd masstree; git rev-list -n1 $$cur^..HEAD 2>/dev/null`; then (echo Updating masstree... 1>&2; cd masstree; git checkout -f master >/dev/null; git pull; cd ..; git submodule update masstree); fi)
endif

ifneq ($(strip $(MASSTREE_CONFIG)),$(strip $(DEP_MASSTREE_CONFIG)))
DEP_MASSTREE_CONFIG := $(shell mkdir -p $(O); echo >$(O)/buildstamp.masstree; echo "DEP_MASSTREE_CONFIG:=$(MASSTREE_CONFIG)" >masstree/_masstree_config.d)
endif

$(O)/buildstamp $(O)/buildstamp.bench $(O)/buildstamp.masstree:
	@mkdir -p $(@D)
	@echo >$@

.PHONY: clean
clean:
	rm -rf out-*
