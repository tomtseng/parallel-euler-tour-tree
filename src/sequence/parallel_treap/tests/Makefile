ROOT_DIR=$(shell git rev-parse --show-toplevel)
include $(ROOT_DIR)/Makefile.common
TARGET=test_treap
OBJS=$(TARGET).o $(SRC_DIR)/sequence/parallel_treap/src/treap.o

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PARALLEL_FLAGS) -c -o $@ $<

-include $(TARGET).d

.PHONY: clean
clean:
	$(RM) \
	  $(OBJS) \
	  $(patsubst %.o,%.d,$(OBJS)) \
          $(BIN_DIR)/$(TARGET) \
