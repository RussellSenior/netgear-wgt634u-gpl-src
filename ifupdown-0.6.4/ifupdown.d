addrfam.c : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
include addrfam.d
inet.defn : ifupdown.nw
	notangle -t8 -R$@ $< >$@
makenwdep.sh : ifupdown.nw
	notangle -R$@ $< >$@
	chmod 755 makenwdep.sh
Makefile : ifupdown.nw
	notangle -t8 -R$@ $< >$@
inet6.defn : ifupdown.nw
	notangle -t8 -R$@ $< >$@
header.h : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
main.c : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
include main.d
defn2man.pl : ifupdown.nw
	notangle -R$@ $< >$@
	chmod 755 defn2man.pl
archlinux.h : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
archlinux.c : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
include archlinux.d
defn2c.pl : ifupdown.nw
	notangle -R$@ $< >$@
	chmod 755 defn2c.pl
config.c : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
include config.d
ipx.defn : ifupdown.nw
	notangle -t8 -R$@ $< >$@
execute.c : ifupdown.nw
	notangle -L -R$@ $< | cpif $@
include execute.d
makecdep.sh : ifupdown.nw
	notangle -R$@ $< >$@
	chmod 755 makecdep.sh
