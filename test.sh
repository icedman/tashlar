#!/bin/sh

./build/tashlar -s ./tests/test.js
diff ./tests/results ./tests/expected
