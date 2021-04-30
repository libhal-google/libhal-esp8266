SJ2_TEST_FLAGS := -g --coverage -fPIC -fexceptions -fno-inline -fno-builtin \
                  -fno-inline-small-functions -fno-default-inline \
                  -fkeep-inline-functions -fno-elide-constructors \
                  -fprofile-arcs -ftest-coverage \
                  -fdiagnostics-color -fno-stack-protector \
                  -fsanitize=address \
                  -Wall -Wno-variadic-macros -Wextra -Wshadow -Wno-main \
                  -Wno-missing-field-initializers \
                  -Wfloat-equal -Wundef -Wno-format-nonliteral \
                  -Wdouble-promotion -Wswitch -Wnull-dereference -Wformat=2 \
                  -D PLATFORM=host -O0 -std=c++20 -MMD -MP \
                  -pthread

BUILD_DIRECTORY   := build
LIBRARY_DIRECTORY := $(shell basename -s .git \
                             `git config --get remote.origin.url`)
TEST_SOURCE       := $(shell find $(LIBRARY_DIRECTORY)/ -name "*.test.cpp")
TEST_EXECUTABLE   := $(patsubst %,$(BUILD_DIRECTORY)/%.exe, $(TEST_SOURCE))

.PHONY: run-tests tests

$(BUILD_DIRECTORY)/%.exe: %
	@mkdir -p "$(dir $@)"
	@printf 'Building Source  (C++): $<\n'
	@g++-10 $(SJ2_TEST_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -I ./ "$<" -o "$@"

tests: $(TEST_EXECUTABLE)
	@echo GENERATING TESTS DONE!

run-tests:
	@./scripts/test_runner.sh $(TEST_EXECUTABLE)
