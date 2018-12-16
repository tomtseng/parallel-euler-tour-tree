ROOT_DIR=$(shell git rev-parse --show-toplevel)
include $(ROOT_DIR)/Makefile.common
TARGET=benchmark_dynamic_trees_splay_tree_ett
OBJS=$(TARGET).o \
     $(SRC_DIR)/dynamic_trees/euler_tour_tree/src/splay_tree_ett.o \
     $(SRC_DIR)/sequence/splay_tree/src/splay_tree.o

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(TARGET).d

.PHONY: clean
clean:
	$(RM) \
	  $(OBJS) \
	  $(patsubst %.o,%.d,$(OBJS)) \
          $(BIN_DIR)/$(TARGET) \
