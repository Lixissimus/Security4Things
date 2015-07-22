DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += k-means.c hamming.c
CONTIKI_PROJECT=light-app
all: $(CONTIKI_PROJECT)
CONTIKI=contiki
include $(CONTIKI)/Makefile.include