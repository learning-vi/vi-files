## Process this file with automake to produce Makefile.in

nobase_dist_vim_DATA = clewn.vim .clewn_keys 	\
	doc/clewn.txt 				\
	macros/clewn_mappings.vim 		\
	syntax/gdbvar.vim



help-tags-generation:
	@echo "clewn help tags file generation"
	@cd "$(DESTDIR)$(vimdir)/doc"; vim -u NONE -esX -c "helptags ." -c quit

install-data-hook: help-tags-generation

uninstall-hook: help-tags-generation

