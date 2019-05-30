all: minisat cryptominisat abcdsat glucose mapleglucose

minisat:
	mkdir -p minisat
	$(MAKE) -C minisat-sources config prefix=../minisat
	$(MAKE) -C minisat-sources install

cryptominisat:
	mkdir -p cryptominisat-5.0.1/build
	cd cryptominisat-5.0.1/build; cmake ..
	$(MAKE) -C cryptominisat-5.0.1/build

abcdsat:
	$(MAKE) -C abcdsat_i17

riss:
	$(MAKE) -C riss_7

glucose:
	cd glucose-syrup-4.1/simp; $(MAKE) -C . libr

mapleglucose:
	cd MapleGlucose/simp; $(MAKE) -C . libr

clean:
	rm -rf minisat
	rm -rf cryptominisat-5.0.1/build
	rm -rf abcdsat_i17/abcdsat-_i17/build
	rm -f abcdsat_i17/libipasirabcdsat_i17.a
	rm -f glucose-syrup-4.1/*/*.or
	rm -f glucose-syrup-4.1/simp/depend.mk
	rm -f glucose-syrup-4.1/simp/glucose_static
	rm -f glucose-syrup-4.1/simp/*.a
	rm -f MapleGlucose/*/*.or
	rm -f MapleGlucose/simp/*.a
	rm -f MapleGlucose/simp/depend.mk

.PHONY: all minisat cryptominisat abcdsat riss glucose clean mapleglucose
