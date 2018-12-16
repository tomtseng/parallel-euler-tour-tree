ROOT_DIR=$(shell git rev-parse --show-toplevel)
include $(ROOT_DIR)/Makefile.common

BINARY_OBJS = generate_binary_tree_graph.o graph_io.o
PATH_OBJS = generate_path_graph.o graph_io.o
RECURSIVE_OBJS = generate_recursive_tree_graph.o graph_io.o
STAR_OBJS = generate_star_graph.o graph_io.o

all: \
  $(BIN_DIR)/generate_binary_tree_graph \
  $(BIN_DIR)/generate_path_graph \
  $(BIN_DIR)/generate_recursive_tree_graph \
  $(BIN_DIR)/generate_star_graph

$(BIN_DIR)/generate_binary_tree_graph: $(BINARY_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/generate_path_graph: $(PATH_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/generate_recursive_tree_graph: $(RECURSIVE_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/generate_star_graph: $(STAR_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(PARALLEL_FLAGS) -c -o $@ $<

.PHONY: clean

clean:
	$(RM) \
	  $(BINARY_OBJS) \
	  $(PATH_OBJS) \
	  $(RECURSIVE_OBJS) \
	  $(STAR_OBJS) \
	  $(patsubst %.o,%.d,$(BINARY_OBJS)) \
	  $(patsubst %.o,%.d,$(PATH_OBJS)) \
	  $(patsubst %.o,%.d,$(RECURSIVE_OBJS)) \
	  $(patsubst %.o,%.d,$(STAR_OBJS)) \
	  $(BIN_DIR)/generate_binary_graph \
	  $(BIN_DIR)/generate_path_graph \
	  $(BIN_DIR)/generate_recursive_tree_graph \
	  $(BIN_DIR)/generate_star_graph
