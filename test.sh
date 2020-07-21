#!/bin/sh

./build/tashlar -s ./tests/test.lua
diff ./tests/results ./tests/expected
