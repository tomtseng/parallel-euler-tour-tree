all:
	make -C data/src

.PHONY: clean
clean:
	make clean -C data/src
	find . -name "*.o" -type f -delete
	find . -name "*.d" -type f -delete
	rm -f data/graphs/*
