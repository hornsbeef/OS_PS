CC		:= gcc
CFLAGS 	?= -g -std=c11 -Wall -Wextra -pthread
CPPFLAG ?= 
LDFLAGS += -g -lrt

# settings
TASK_NUM := 2

EXECS := task2

# specifies if special zip rule should be used
CUSTOM_ZIP_RULE := 0
# following have only effect if custom zip rule is enabled
ZIP_SRC := 1
ZIP_MAKEFILE := 1
# additional to src
ADDITIONAL_FILES_TO_ZIP := 
MAKEFILE_NAME := makefile

# GENERATED - DONT CHANGE
OBJS := $(addsuffix .o,$(EXECS))
SRC := $(addsuffix .c,$(EXECS))
TASK_NAME := task$(TASK_NUM)

ifeq ($(CUSTOM_ZIP_RULE),1)
  PATHS_TO_ZIP := $(addprefix $(TASK_NAME)/,$(ADDITIONAL_FILES_TO_ZIP))
  ifeq ($(ZIP_SRC),1)
    #PATHS_TO_ZIP += $(addprefix $(PROJECT_NAME)/,$(SRC))
    PATHS_TO_ZIP += $(TASK_NAME)/*.c
  endif
  ifeq ($(ZIP_MAKEFILE),1)
    PATHS_TO_ZIP += $(TASK_NAME)/$(MAKEFILE_NAME)	
  endif
# no custom rule -> zip everything
else
  PATHS_TO_ZIP := $(TASK_NAME)/*
endif

.PHONY: all
.PHONY: clean
.PHONY: run
.PHONY: zip

all: $(EXECS)

$(EXECS): %: %.o
	@echo "Linking $*"
	$(CC) $(LDFLAGS) $< $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.c
	@echo "Compiling $*"
	$(CC) -c $(CFLAGS) $(CPPFLAG) $< -o $@

clean:
	@echo "Cleaning up..."
	@$(RM) $(EXECS) $(OBJS)
	@echo "Done."

zip:
ifdef ARCHIVE
	@(cd ..; zip -u $(ARCHIVE) $(PATHS_TO_ZIP))
else
	$(error error: ARCHIVE variable is not set)
endif	


