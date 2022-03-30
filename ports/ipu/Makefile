.PHONY: make_build_dir gen_hdrs clean

BUILDDIR=build

PY_CORE_OBJ = $(BUILDDIR)/poppy.gp
PORT_OBJ = $(BUILDDIR)/codelets.gp 

OBJ += $(PORT_OBJ)$(PY_CORE_OBJ)

HARDCODEVARS=-DNDEBUG -DFFCONF_H=\"lib/oofatfs/ffconf.h\" -DMICROPY_ROM_TEXT_COMPRESSION=1
WARNFLAGS=-Wall -Wdouble-promotion -Wfloat-conversion -Werror
INC= -I../.. -Ibuild -I../../py -I.
LLVMFLAGS=-X -ffunction-sections -X -fdata-sections
POPCFLAGS=--target=ipu2 -Os $(INC) $(WARNFLAGS) $(HARDCODEVARS) $(LLVMFLAGS)

default: make_build_dir gen_hdrs $(BUILDDIR)/runcodelets $(OBJ)
	popc -Wl,-Map=build/firmware.elf.map,--cref -Wl,--gc-sections $(POPCFLAGS) -o $(BUILDDIR)/main.gp $(OBJ)

make_build_dir:
	mkdir -p $(BUILDDIR)/py $(BUILDDIR)/shared/runtime

gen_hdrs:
	make -f GENMakefile build/genhdr/compressed.data.h
	make -f GENMakefile build/genhdr/qstrdefs.generated.h
	make -f GENMakefile build/genhdr/moduledefs.h

$(BUILDDIR)/runcodelets: runcodelets.cpp
	g++ runcodelets.cpp -std=c++17 -O2 -lpoplar -o $(BUILDDIR)/runcodelets

$(BUILDDIR)/%.gp: %.c
	popc $(POPCFLAGS) -o $@ $< 

$(BUILDDIR)/codelets.gp: codelets.cpp
	./poppyc $(POPCFLAGS) -Wno-unused-function -o $@ -i $< 


clean: 
	rm -r $(BUILDDIR)