all: solvers src

solvers:
	$(MAKE) -C solvers all

src: solvers
	$(MAKE) -C src all
	cp src/triangulator triangulator

clean:
	$(MAKE) -C src clean
	$(MAKE) -C solvers clean

.PHONY: all solvers src clean
