SUBDIRS += client
SUBDIRS += server
SUBDIRS += epoll-client
SUBDIRS += udp-read-trend
SUBDIRS += udp-mmsg-trend

.PHONY: $(SUBDIRS)

all: $(SUBDIRS)
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

clean:
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done
