MAKEFLAGS += --silent
.PHONY: clean clean_old $(SUBDIRS) zip number zipname


SUBDIRS := $(wildcard */.)

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir; \
	done


number := $(shell pwd|rev|cut -c 1-2|rev)
zipname := exc$(number)_csbb5525.zip
zip_ignored := "*.idea*" "__MACOSX" ".DS_Store" "./README.md" ".clang-format"

zip:
	@echo "exercise number: "$(number)
	@echo "checking if "$(zipname)" already exists and removing it..."
	-rm -f $(zipname)
	@echo "creating: "$(zipname)
	-zip -r $(zipname) . -x $(zip_ignored)

zip_remove:
	@echo "removing "$(zipname)
	-rm -f $(zipname)

#https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory
#https://linuxcommand.org/lc3_man_pages/forh.html
#https://linux.die.net/man/1/zip

#not in production below this line

clean_old:
	cd ./task1&& $(MAKE) clean
