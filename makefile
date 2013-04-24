# -----Begin user-editable area-----

#include $(wildcard $(OUTDIR)/*.d)

# -----End user-editable area-----

# Make command to use for dependencies
MAKE=gmake

# If no configuration is specified, "Debug" will be used
ifndef "CFG"
CFG=Debug
endif

#
# Configuration: Debug
#
ifeq "$(CFG)" "Debug"
OUTDIR=Debug
OUTFILE=collect_p_f_data
CFG_INC=-I/extra/boost/boost-1.53_gcc-4.8/ -I/home/dpriedel/projects/workspace/app_framework/include/
CFG_LIB=-L/extra/boost/boost-1.53_gcc-4.8/lib -lboost_system-d
RPATH_LIB=-Xlinker -rpath -Xlinker /extra/gcc/gcc-4.8/lib64 -Xlinker -rpath -Xlinker /extra/boost/boost-1.53_gcc-4.8/lib
CFG_OBJ=
COMMON_OBJ=$(OUTDIR)/collect_p_f_data.o
OBJ=$(COMMON_OBJ) $(CFG_OBJ)

COMPILE=/extra/gcc/gcc-4.8/bin/g++ -c  -x c++  -O0  -g3 -std=c++11 -fPIC -o "$(OUTDIR)/$(*F).o" $(CFG_INC) "$<" -march=native -MMD  
LINK=/extra/gcc/gcc-4.8/bin/g++  -g -o "$(OUTFILE)" $(OBJ) $(CFG_LIB) -Wl,-E $(RPATH_LIB)

# Pattern rules
$(OUTDIR)/%.o : ./src/%.cpp
	$(COMPILE)

# Build rules
all: $(OUTFILE)

$(OUTFILE): $(OUTDIR)  $(OBJ)
	$(LINK)

$(OUTDIR):
	mkdir -p "$(OUTDIR)"

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	rm -f $(OUTFILE)
	rm -f $(OBJ)

# Clean this project and all dependencies
cleanall: clean
endif

