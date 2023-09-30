# 编译
.PHONY: build
build:
	blade build ...

# 生成 clangd compile_commands.json
.PHONY: clangd
clangd:
	rm -rf build64_release
	bear -- blade build ...

# 全量单测
.PHONY: test
test:
	blade test ... --full-test --run-unrepaired-tests --coverage

# 单测覆盖率
.PHONY: coverage
coverage:
	mkdir -p coverage_reports
	gcovr -r . --html --html-details -o coverage_reports/coverage_report.html
	
# 单测覆盖率 server
.PHONY: coverage-server
coverage-server: coverage
	cd coverage_reports && python3 -m http.server 8080
