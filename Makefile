SHELL := /bin/bash

PROJECTS ?=
LOCAL_PATH ?= /usr/local/bin/
ADD_LOCAL_FLAGS ?=

.PHONY: all build add-local test lint

all: build test lint

build:
	./.scripts/build_all.sh $(PROJECTS)

add-local:
	./.scripts/add_to_local.sh $(ADD_LOCAL_FLAGS) $(LOCAL_PATH) -y

test:
	./.scripts/test_all.sh

lint:
	./.scripts/lint_all.sh $(PROJECTS)
