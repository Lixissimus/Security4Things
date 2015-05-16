DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT=light-app
all: $(CONTIKI_PROJECT)
CONTIKI=contiki
include $(CONTIKI)/Makefile.include