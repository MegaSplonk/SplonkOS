PROJECTS =                                                                     \
	test-adc                                                                   \
	test-attiny                                                                \
	test-ec11                                                                  \
	test-matrix                                                                \
	test-nano                                                                  \
	test-usb                                                                   \

GLOBAL_DEPS = lib Base.mk .clang-format .github Makefile flake.nix flake.lock

.PHONY: all
all:
	@$(MAKE) -j --no-print-directory .allhex

re: fclean all

.PHONY: fclean clean
fclean: clean
clean:
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) clean;\
	)

filelist:
	@$(foreach p,$(PROJECTS),\
		printf "Filelist: $(p)\n";\
		$(MAKE) --no-print-directory -C $(p) filelist;\
	)


.PHONY: bear
bear:
	@printf "Rebuilding everything to get compile commands\n"
	@bear -- make -j --no-print-directory .ccdb
	@printf "Done !\n"

.PHONY: .ccdb
.ccdb:
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) USE_ABSOLUTE_PATH=1 --no-print-directory -C $(p) clean;\
	)
	@$(foreach p,$(PROJECTS),\
		printf "Clean: $(p)\n";\
		$(MAKE) USE_ABSOLUTE_PATH=1 --no-print-directory -C $(p) hex;\
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

.PHONY: list-projects
list-projects:
	@echo "$(PROJECTS)"

.PHONY: list-global-deps
list-global-deps:
	@echo "$(GLOBAL_DEPS)"

.PHONY: check-format
check-format:
	@clang-format --Werror --dry-run $$(git ls-files | grep -E '\.(c|h)$$')
