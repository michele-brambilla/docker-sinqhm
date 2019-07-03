#-------------------------------------------------------------------------------
# makefile for Linux/RTAI SINQHM server
#-------------------------------------------------------------------------------

SHELL = /bin/sh

fil_target=user_x86

#-------------------------------------------------------------------------------
# Path to source files
#-------------------------------------------------------------------------------

SRC_PATH=../src


vpath %.c   $(SRC_PATH)
vpath %.cpp $(SRC_PATH)
vpath %.h   $(SRC_PATH)
vpath %.s   $(SRC_PATH)


#-------------------------------------------------------------------------------
# Filler Application
#-------------------------------------------------------------------------------

FIL_MAIN = filler_main.c

FIL_SRC = datashm.c          \
          controlshm.c       \
          debugshm.c         \
          lwlpmc.c           \
          process_dig.c      \
          process_hrpt.c     \
          process_psd.c      \
          process_tof.c      \
          process_tofmap.c   \
          process_sans2.c    \
          process_common.c   \
          rawdata.c	\
	zeromq.c	\
	process_rita2.c	\
	process_camea.c	\
	log.c		\
	mongo.c		\
	sinqhmlog.c	\
	cJSON.c


# FIL_INLINE = -D_INLINE

FIL_DIR = ./rt

FIL_MAIN_OBJ = $(FIL_MAIN:%.c=$(FIL_DIR)/%.o)
FIL_OBJ      = $(FIL_SRC:%.c=$(FIL_DIR)/%.o)

FIL_LIB = -lzmq

#-------------------------------------------------------------------------------
# Server Application
#-------------------------------------------------------------------------------

APPWEB_VER = 2.4.2
#APPWEB_VER = 2.0.4


APPWEB_DEF = -D_DEBUG  -DLINUX -D_REENTRANT -DAPPWEB_VER_$(subst .,_,$(APPWEB_VER))
APPWEB_LIB = appWebStatic-$(APPWEB_VER)
APPWEB_INC = ../include/appWeb-$(APPWEB_VER)

SRV_SRC = sinqhmegi.c    \
          egiadapter.c   \
          sinqhmsrv.c    \
          datashm.c      \
          controlshm.c   \
          nxdataset.c    \
          stptok.c       \
          debugshm.c     \
          lwlpmc.c

SRV_SRC_PP = appWrapper.cpp

SRV_DIR = ./ua

SRV_APP = $(SRV_DIR)/sinqhmegi

SRV_OBJ = $(SRV_SRC:%.c=$(SRV_DIR)/%.o)  $(SRV_SRC_PP:%.cpp=$(SRV_DIR)/%.o)

SRV_DBG = -g

#-------------------------------------------------------------------------------
# VME LWL TestGen
#-------------------------------------------------------------------------------

VLT_SRC = vmelwl.c vme_testgen.c

VLT_DIR = ./vlt

VLT_APP = $(VLT_DIR)/vmetgen

VLT_OBJ = $(VLT_SRC:%.c=$(VLT_DIR)/%.o)


#-------------------------------------------------------------------------------
# target architecture dependant settings
#-------------------------------------------------------------------------------



#----------------------------------------
# filler in linux user space
#----------------------------------------
ifeq ($(fil_target),user_x86)

TARGET_DEFINED = 1
APPWEB_LIB = appwebStatic

#
# (Non-)Realtime Application
#

FIL_APP = $(FIL_DIR)/sinqhm_filler

FIL_INC = -I. \
         -I ../include \
	-I $(APPWEB_INC) \
	$(MONGO_INC)

FIL_DEF = -DAPP_FILLER -DARCH_X86 -DFILLER_USER $(APPWEB_DEF)
FIL_FLG =  $(SRV_DBG) -Wall -DHAVE_0MQ -DUSE_MONGO_LOG
FIL_LIB = -lm -lzmq $(MONGO_LIB)

#
# Server Application
#
SRV_INC = -I. \
         -I ../include  -I $(APPWEB_INC) $(MONGO_INC)
SRV_DEF = -DAPP_SERVER -DARCH_X86 -DFILLER_USER $(APPWEB_DEF)
SRV_FLG =  $(SRV_DBG) -Wall -DHAVE_0MQ -DUSE_MONGO_LOG
SRV_LIB = -L../lib/x86_64 -lmxml  -l$(APPWEB_LIB) -lpthread -ldl $(MONGO_LIB)

endif


#-------------------------------------------------------------------------------
# Mongodb extra
#-------------------------------------------------------------------------------

MONGO_INC = `pkg-config --cflags libmongoc-1.0`
MONGO_LIB = -lmongoc-1.0 -lbson-1.0 -lssl3 #-lsasl -lcrypto

#-------------------------------------------------------------------------------
# Check for valid fil_target
#-------------------------------------------------------------------------------

ifndef TARGET_DEFINED
ifndef fil_target
$(error ERROR: variable $$(fil_target) not defined)
else
$(error ERROR: invalid target: $$(fil_target)="$(fil_target)")
endif
endif


################################################################################
#
#  Rules
#
################################################################################


ifneq ($(fil_target),user_x86)
all:  $(FIL_APP) $(SRV_APP) $(VLT_APP) install
else
all:  $(FIL_APP) $(SRV_APP) install

mongotest: mongotest.c mongo.c log.c log.h mongo.h
	gcc -O0 -ggdb -fomit-frame-pointer -o mongotest mongotest.c $(MONGO_INC) $(MONGO_LIB) log.c mongo.c -DUSE_MONGO_LOG
endif
#  clean


vmelwl:  $(VLT_APP)
	echo 	-cp $(VLT_APP) /tftpboot/nfs/home/theidel

#-------------------------------------------------------------------------------

.PHONY : clean
clean:
	-rm -f $(SRV_DIR)/*.o $(SRV_DIR)/*.d $(SRV_APP) $(FIL_DIR)/*.o $(FIL_DIR)/*.d $(FIL_APP)
	-rm -f $(VLT_DIR)/*.o $(VLT_DIR)/*.d $(VLT_APP)
#-------------------------------------------------------------------------------

.PHONY : install

ifneq ($(fil_target),user_x86)
install: $(FIL_APP) $(SRV_APP) $(VLT_APP)
	-cp $(FIL_APP) ../distrib/sinqhm
	-cp $(SRV_APP) ../distrib/sinqhm
	-cp $(VLT_APP) ../distrib/sinqhm
	ppc_60x-strip ../distrib/sinqhm/sinqhmegi
	ppc_60x-strip ../distrib/sinqhm/vmetgen
else
install: $(FIL_APP) $(SRV_APP)
	-cp $(FIL_APP) ../distrib/sinqhm
	-cp $(SRV_APP) ../distrib/sinqhm
endif

ifeq "$(fil_target)" "rtai_ppc"
#	-cp -r ../distrib/server   /tftpboot/nfs/home/theidel
#	-cp ../src/*          /tftpboot/nfs/home/src
endif

#-------------------------------------------------------------------------------

$(FIL_DIR):
	mkdir -p $(FIL_DIR)

# recompile FIL_MAIN_OBJ if any FIL_OBJ has changed, since Date and Time
# is compiled int FIL_MAIN_OBJ
$(FIL_MAIN_OBJ) : $(FIL_MAIN) $(FIL_OBJ)
	$(CC) -c $(FIL_INLINE) $(FIL_INC) $(FIL_DEF) $(FIL_FLG)    $< -o $@

$(FIL_DIR)/%.o : %.c
	$(CC) -c $(FIL_INC) $(FIL_DEF) $(FIL_FLG)    $< -o $@

$(FIL_APP):  $(FIL_DIR) $(FIL_MAIN_OBJ) $(FIL_OBJ)
	$(CC) $(FIL_MAIN_OBJ) $(FIL_OBJ) $(FIL_LIB) -o $@

#-------------------------------------------------------------------------------

$(SRV_DIR):
	mkdir -p $(SRV_DIR)

$(SRV_DIR)/%.o : %.c
	$(CC) -c $(SRV_INC) $(SRV_DEF) $(SRV_FLG)    $< -o $@

$(SRV_DIR)/%.o : %.cpp
	$(CXX) -c $(SRV_INC) $(SRV_DEF) $(SRV_FLG)    $< -o $@

$(SRV_APP): $(SRV_DIR) $(SRV_OBJ)
	$(CXX)  $(SRV_DBG) $(SRV_OBJ) $(SRV_LIB) -o $@

#---------------------------------------------------------------------------
# generate dependency file for each C source file
#---------------------------------------------------------------------------

SED_FIL_DIR = $(subst /,\/,$(FIL_DIR))\/

$(FIL_DIR)/%.d : %.c
	$(SHELL) -ec 'mkdir -p $(FIL_DIR) && $(CC) -M $(FIL_INC) $(FIL_DEF) $(FIL_FLG) $< \
		| sed '\''s/^$*\.o[	 ]*:/$(SED_FIL_DIR)$*.o $(SED_FIL_DIR)$*.d : /g'\'' > $@'

SED_SRV_DIR = $(subst /,\/,$(SRV_DIR))\/

$(SRV_DIR)/%.d : %.c
	$(SHELL) -ec 'mkdir -p $(SRV_DIR) && $(CC) -M $(SRV_INC) $(SRV_DEF) $(SRV_FLG) $< \
		| sed '\''s/^$*\.o[	 ]*:/$(SED_SRV_DIR)$*.o $(SED_SRV_DIR)$*.d : /g'\'' > $@'

$(SRV_DIR)/%.d : %.cpp
	$(SHELL) -ec 'mkdir -p $(SRV_DIR) && $(CC) -M $(SRV_INC) $(SRV_DEF) $(SRV_FLG) $< \
		| sed '\''s/^$*\.o[	 ]*:/$(SED_SRV_DIR)$*.o $(SED_SRV_DIR)$*.d : /g'\'' > $@'

include $(FIL_OBJ:.o=.d) $(SRV_OBJ:.o=.d)

#-------------------------------------------------------------------------------
