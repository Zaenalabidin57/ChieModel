# ChieModel Makefile
# Created for GitHub project

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -march=native
LDLIBS = -lSDL2 -lSDL2_image -lSDL2_ttf

# Program name
PROGRAM = ChieModel

# Source files
SRCS = ChieModel.cpp embedded_models.cpp
OBJS = $(SRCS:.cpp=.o)

# Installation paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SHAREDIR = $(PREFIX)/share/$(PROGRAM)
APPDIR = $(PREFIX)/share/applications

# Default target
all: embedded_models.cpp $(PROGRAM)

# Generate embedded models
embedded_models.cpp: model generate_embedded_models.py
	python3 generate_embedded_models.py model embedded_models.cpp
	@echo "Generated embedded_models.cpp with model data"

# Main build
$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

# Generic rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
.PHONY: clean
clean:
	rm -f $(OBJS) $(PROGRAM) embedded_models.cpp $(PROGRAM).desktop

# Create desktop entry file
$(PROGRAM).desktop:
	@echo "[Desktop Entry]" > $(PROGRAM).desktop
	@echo "Name=ChieModel" >> $(PROGRAM).desktop
	@echo "Comment=Chie 2D Virtual Avatar System" >> $(PROGRAM).desktop
	@echo "Exec=$(BINDIR)/$(PROGRAM)" >> $(PROGRAM).desktop
	@echo "Terminal=false" >> $(PROGRAM).desktop
	@echo "Type=Application" >> $(PROGRAM).desktop
	@echo "Categories=Graphics;Utility;" >> $(PROGRAM).desktop
	@echo "Keywords=avatar;virtual camera;" >> $(PROGRAM).desktop

# Install
.PHONY: install
install: all $(PROGRAM).desktop
	@echo "Installing $(PROGRAM) to $(BINDIR)..."
	@mkdir -p $(BINDIR)
	@mkdir -p $(APPDIR)
	@cp $(PROGRAM) $(BINDIR)/
	@cp $(PROGRAM).desktop $(APPDIR)/
	@echo "Installation complete. Run with: $(PROGRAM) or use the application menu."

# Uninstall
.PHONY: uninstall
uninstall:
	@echo "Uninstalling $(PROGRAM)..."
	@rm -f $(BINDIR)/$(PROGRAM)
	@rm -f $(APPDIR)/$(PROGRAM).desktop
	@echo "Uninstallation complete."

# Help
.PHONY: help
help:
	@echo "ChieModel - Virtual Avatar System"
	@echo "Available targets:"
	@echo "  all       - Build the ChieModel application (default)"
	@echo "  clean     - Remove build files"
	@echo "  install   - Install ChieModel to $(BINDIR)"
	@echo "  uninstall - Remove ChieModel from your system"
	@echo "  help      - Display this help message"
	@echo ""
	@echo "Usage commands:"
	@echo "  ./$(PROGRAM)         - Run with virtual camera output"
	@echo "  ./$(PROGRAM) --window - Run in window-only mode"
	@echo "  ./$(PROGRAM) --help   - Show application help"
