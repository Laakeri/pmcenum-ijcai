SOLVERS = ../solvers

RISSIPASIRLIB = -L $(SOLVERS)/riss_7/ -lipasirriss_7
ABCDIPASIRLIB = -L $(SOLVERS)/abcdsat_i17/ -lipasirabcdsat_i17
IPASIRLIB = $(ABCDIPASIRLIB)

CC = g++
CFLAGS = -std=c++11 -O2 -Wall -Wextra -Wshadow -g -Wfatal-errors -Wno-sign-compare -Wno-literal-suffix -fPIC
INC = -I $(SOLVERS)/minisat/include/ -I $(SOLVERS)/cryptominisat-5.0.1/build/include/ -I $(SOLVERS)/glucose-syrup-4.1/include/ -I $(SOLVERS)/MapleGlucose/include/
LIB = -lm -lz -lpthread -L $(SOLVERS)/glucose-syrup-4.1/simp/ -l_release -L $(SOLVERS)/MapleGlucose/simp/ -l_mapleglucose_release -L $(SOLVERS)/minisat/lib/ -lminisat -L $(SOLVERS)/cryptominisat-5.0.1/build/lib/ -lcryptominisat5 -Wl,-rpath=$(CURDIR)/$(SOLVERS)/minisat/lib/:$(CURDIR)/$(SOLVERS)/cryptominisat-5.0.1/build/lib/ $(IPASIRLIB)
OBJFILES = setcover.o comb_enum.o solver.o hypertreewidthpreprocessor.o treewidthpreprocessor.o asp_enumerator.o enumerator.o sat_interface.o graph.o minisat_interface.o utils.o io.o staticset.o matrix.o glucose_interface.o cryptominisat_interface.o ipasir_interface.o mcs.o bt_algorithm.o fixed_size_enumerator.o cardinality_constraint_builder.o hypergraph.o fixed_size_hyper_enumerator.o tests.o mapleglucose_interface.o

all: triangulator

triangulator: triangulator.o $(OBJFILES)
	$(CC) $(INC) $(CFLAGS) triangulator.o $(OBJFILES) -o triangulator $(LIB)

%.o: %.cpp
	$(CC) $(INC) $(CFLAGS) -MMD -c $< -o $@

-include *.d

clean:
	rm -f *.o *.d

.PHONY: all clean
