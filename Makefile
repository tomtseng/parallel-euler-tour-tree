all: 
	echo "Making everything is not implemented."

.PHONY: clean
clean:
	find . -name "*.o" -type f -delete
	find . -name "*.d" -type f -delete
	rm -rf bin/*
	rm -rf `find -type d -name 'times'`
	rm -f src/dynamic_trees/benchmarks/data/graphs/*
