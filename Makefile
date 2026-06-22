PROJECTS = usb test

.PHONY: all
all:
	$(MAKE) -j --no-print-directory .allhex

.PHONY: fclean clean
fclean: clean
clean:
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) clean;\
	)


.PHONY: bear
bear:
	@printf "Rebuilding everything to get compile commands\n"
	@bear -- make -j --no-print-directory .ccdb 'relpath=$$(1)'
	@printf "Done !\n"

.PHONY: .ccdb
.ccdb:
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) clean;\
	)
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) hex;\
	)

.PHONY: .allhex
.allhex:
	@$(foreach p,$(PROJECTS),\
		printf "Building: $(p) [only hex]\n";\
		$(MAKE) --no-print-directory -C $(p) hex;\
	)

.PHONY: format
format:
	tree -iFf  | rg '\.[ch]$$' | xargs clang-format -i
