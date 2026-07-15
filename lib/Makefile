SRC_DIR=./src/

filelist:
	@printf '%-78s\\\n' "LIB_SRC_FILES =" > ./Filelist.mk
	@find "./src/" -type f -name '*.c' \
		| sed -e 's|^./src/||' -e 's|\.c$$||' \
		| sort \
		| while read f; do printf '%-78s\\\n' "$$f"; done >> ./Filelist.mk
	@echo "" >> ./Filelist.mk
	@printf "Filelist: lib\n"
