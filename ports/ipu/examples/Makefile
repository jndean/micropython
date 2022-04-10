.PHONY: clean

EXAMPLES = example1 example2 example3 example4

CODELETS = $(addsuffix _codelets.gp, $(EXAMPLES))

PREPROCESSOR = ./example3

all: $(EXAMPLES) $(CODELETS)

example1: example1_host.cpp example1_codelets.gp
	g++ example1_host.cpp -std=c++17 -lpoplar -o $@
	
example2: example2_host.cpp example2_codelets.gp
	g++ example2_host.cpp -std=c++17 -lpoplar -o $@

example3: example3_host.cpp example3_codelets.gp
	g++ example3_host.cpp -std=c++17 -lpoplar -o $@

example4: example4_host.cpp example4_codelets.gp
	g++ example4_host.cpp -std=c++17 -lpoplar -o $@

%.gp : %.cpp
	libpoppy/ports/ipu/poppyc -i $< -o $@ --external_preprocessor $(PREPROCESSOR)

clean:
	rm example1 example2 example4 example1_codelets.gp example2_codelets.gp example4_codelets.gp