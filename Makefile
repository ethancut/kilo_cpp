CPPC = clang++
CPPFLAGS = -Wall -Wextra -pedantic -std=c++23

.PHONY: kilo run clean

kilo: src/main.cpp
	$(CPPC) src/main.cpp -o kilo.exe $(CPPFLAGS)

run: kilo
	./kilo.exe $(ARGS)
clean:
	@pwsh -NoProfile -Command "if (Test-Path kilo.exe) { Remove-Item kilo.exe }"