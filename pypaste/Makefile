BIN := $(notdir $(CURDIR))

build:
	@cargo build --release
	@cp target/release/$(BIN) ./$(BIN)


test:
	@cargo test

clean:
	@rm -f ./$(BIN)
	@cargo clean
