# The top level Makefile

all:
	cd server && $(MAKE)
	cd client && $(MAKE)
	@echo
	@echo "*********************************************"
	@echo "All file are built"
	@echo "Change to bin/ dir to run binaries"
	@echo "*********************************************"
	@echo
clean:
	cd server && $(MAKE) clean
	cd client && $(MAKE) clean
