# -----Begin user-editable area-----

include $(wildcard $(OUTDIR)/*.d)

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
CFG_INC=-I/extra/boost/boost-1.53_gcc-4.8/ -I/extra/gcc/decNumber -I/home/dpriedel/projects/workspace/app_framework/include/
CFG_LIB=-liberty -L/extra/boost/boost-1.53_gcc-4.8/lib -lboost_system-d -lboost_filesystem-d -lboost_program_options-d
RPATH_LIB=-Xlinker -rpath -Xlinker /extra/gcc/gcc-4.8/lib64 -Xlinker -rpath -Xlinker /extra/boost/boost-1.53_gcc-4.8/lib
CFG_OBJ=
OBJ=$(OUTDIR)/collect_p_f_data.o $(OUTDIR)/CApplication.o $(OUTDIR)/ErrorHandler.o $(OUTDIR)/TException.o \
	$(OUTDIR)/decContext.o $(OUTDIR)/decQuad.o

#OBJ=$(COMMON_OBJ) $(CFG_OBJ)

COMPILE=/extra/gcc/gcc-4.8/bin/g++ -c  -x c++  -O0  -g3 -std=c++11 -fPIC -o "$(OUTDIR)/$(*F).o" $(CFG_INC) "$<" -march=native -MMD  
LINK=/extra/gcc/gcc-4.8/bin/g++  -g -o "$(OUTFILE)" $(OBJ) $(CFG_LIB) -Wl,-E $(RPATH_LIB)

# Pattern rules
# $(OUTDIR)/%.o : ./src/%.cpp 
$(OUTDIR)/collect_p_f_data.o : ./src/collect_p_f_data.cpp ./src/collect_p_f_data.h ./src/DDecimal.h ./src/DDecimal_16.h ./src/DDecimal_32.h
	$(COMPILE)

$(OUTDIR)/CApplication.o : ../app_framework/src/CApplication.cpp ../app_framework/include/CApplication.h
	$(COMPILE)

$(OUTDIR)/ErrorHandler.o : ../app_framework/src/ErrorHandler.cpp ../app_framework/include/ErrorHandler.h
	$(COMPILE)

$(OUTDIR)/TException.o : ../app_framework/src/TException.cpp ../app_framework/include/TException.h
	$(COMPILE)

$(OUTDIR)/decDouble.o : /extra/gcc/decNumber/decDouble.c /extra/gcc/decNumber/decDouble.h
	$(COMPILE)

$(OUTDIR)/decQuad.o : /extra/gcc/decNumber/decQuad.c /extra/gcc/decNumber/decQuad.h
	$(COMPILE)

$(OUTDIR)/decContext.o : /extra/gcc/decNumber/decContext.c /extra/gcc/decNumber/decContext.h
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

