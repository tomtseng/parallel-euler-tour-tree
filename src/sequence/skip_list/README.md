These are sequential skip lists that don't use CAS and don't use the concurrent
allocator. The augmented skip list processes batches of joins and splits in a
naive way, updating augmented values after each join/split.
