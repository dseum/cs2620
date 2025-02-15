all: typecheck test

typecheck:
	@echo "Running type checker"
	@uv run mypy .

test:
	@echo "Running tests"
	@uv run python -m unittest discover -p "*_test.py"

.PHONY: all test check
