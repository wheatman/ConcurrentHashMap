OPT?=3
VALGRIND?=0
SANITIZE?=0
VERIFY?=0

OPENMP?=0
ENABLE_TRACE_TIMER?=1
PARALLEL=0

CFLAGS := -Wall -Wextra -O$(OPT) -g  -std=c++17

LDFLAGS := -lrt -lm -lm -ldl 


ifeq ($(OPENMP),1)
PARALLEL=1
CILK=0
CFLAGS += -fopenmp
ONE_WORKER = OMP_NUM_THREADS=1
else
CILK?=0
endif

ifeq ($(CILK),1)
CFLAGS += -fopencilk
LDFLAGS += -Lx86_64/ -lopencilk
ONE_WORKER = CILK_NWORKERS=1
PARALLEL=1
endif


ifeq ($(PARALLEL),1)
FILEOUT=parallel_run
else
FILEOUT=serial_run
endif




ifeq ($(SANITIZE),1)
ifeq ($(OPENMP),1)
CFLAGS += -fsanitize=undefined,thread -fno-omit-frame-pointer
else
ifeq ($(CILK),1)
CFLAGS += -fsanitize=cilk,undefined -fno-omit-frame-pointer
else
CFLAGS += -fsanitize=undefined,address -fno-omit-frame-pointer
endif
endif
endif

ifeq ($(OPT),3)
CFLAGS += -fno-signed-zeros  -freciprocal-math -ffp-contract=fast -fno-trapping-math  -ffinite-math-only
ifeq ($(VALGRIND),0)
CFLAGS += -march=native #-static
endif
endif



DEFINES := -DOPENMP=$(OPENMP) -DCILK=$(CILK)  -DENABLE_TRACE_TIMER=$(ENABLE_TRACE_TIMER) -DVERIFY=$(VERIFY)

SRC := main.cpp

INCLUDES := hash_table.hpp

.PHONY: all clean tidy

all:  basic 



basic: $(SRC) $(INCLUDES)
	$(CXX) $(CFLAGS) $(DEFINES) -DNDEBUG $(SRC) $(LDFLAGS) -o basic
	cp basic $(FILEOUT)
	objdump -S --disassemble basic > run_basic.dump &


clean:
	rm -f run run_profile run.dump run_basic run.gcda run_basic.dump *.profdata *.profraw test_out/* test basic opt
	rm -f perf.data* parallel_run serial_run

