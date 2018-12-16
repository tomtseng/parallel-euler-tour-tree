ROOT_DIR=$(shell git rev-parse --show-toplevel)
include $(ROOT_DIR)/Makefile.common
TARGET=benchmark_batch_sequence_parallel_augmented_skip_list
OBJS=$(TARGET).o \
     $(SRC_DIR)/sequence/parallel_skip_list/src/augmented_skip_list.o \
     $(SRC_DIR)/sequence/parallel_skip_list/src/skip_list_base.o

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
