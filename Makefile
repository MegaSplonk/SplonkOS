PROJECTS = usb test

all:
	@$(foreach p,$(PROJECTS),\
		printf "Building: $(p) [only hex]\n";\
		$(MAKE) --no-print-directory -C $(p) hex;\
	)

clean:
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) clean;\
	)


bear:
	@printf "Rebuilding everything to get compile commands\n"
	@bear -- make --no-print-directory .ccdb 'relpath=$$(1)'
	@printf "Done !\n"

.ccdb:
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) clean;\
	)
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) hex;\
	)
