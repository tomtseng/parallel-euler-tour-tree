ROOT_DIR=$(shell git rev-parse --show-toplevel)
include $(ROOT_DIR)/Makefile.common

TEST_AUG_OBJS=test_parallel_augmented_skip_list.o \
	      $(SRC_DIR)/sequence/parallel_skip_list/src/augmented_skip_list.o \
	      $(SRC_DIR)/sequence/parallel_skip_list/src/skip_list_base.o
TEST_CON_OBJS=test_parallel_skip_list.o \
	      $(SRC_DIR)/sequence/parallel_skip_list/src/skip_list_base.o

all: \
	$(BIN_DIR)/test_parallel_augmented_skip_list \
	$(BIN_DIR)/test_parallel_skip_list

$(BIN_DIR)/test_parallel_augmented_skip_list: $(TEST_AUG_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/test_parallel_skip_list: $(TEST_CON_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PARALLEL_FLAGS) -c -o $@ $<

-include test_parallel_augmented_skip_list.d test_parallel_skip_list.d

.PHONY: clean
clean:
	$(RM) \
	  $(TEST_AUG_OBJS) \
	  $(TEST_CON_OBJS) \
	  $(patsubst %.o,%.d,$(TEST_AUG_OBJS)) \
	  $(patsubst %.o,%.d,$(TEST_CON_OBJS)) \
          $(BIN_DIR)/test_parallel_augmented_skip_list \
          $(BIN_DIR)/test_parallel_skip_list
