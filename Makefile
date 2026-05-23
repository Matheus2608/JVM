# Makefile para Leitor-Exibidor
# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = 
DEPFLAGS = -MMD -MP

# Diretórios
SRC_DIR = .
BIN_DIR = bin
OBJ_DIR = obj

# Arquivos fonte
SOURCES = main.cpp leitor.cpp parser.cpp exibidor.cpp
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)
EXECUTABLE = $(BIN_DIR)/leitor-exibidor

# Test sources and bins
TEST_SOURCES = $(wildcard tests/*.cpp)
TEST_NAMES = $(notdir $(basename $(TEST_SOURCES)))
TEST_BINS = $(addprefix $(BIN_DIR)/tests/,$(TEST_NAMES))

# Targets
.PHONY: all clean run help

all: $(EXECUTABLE)

# Compilação do executável
$(EXECUTABLE): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✓ Compilação concluída: $@"

# Compilação dos arquivos objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
	@echo "✓ Compilado: $<"

# Cria diretórios se não existirem
$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@



# Diretório para bins de testes
$(BIN_DIR)/tests:
	@mkdir -p $@

# Executa o programa
run: all
	@echo "Executando $(EXECUTABLE)..."
	@./$(EXECUTABLE) $(ARGS)

# Compila e executa testes em tests/ (linka os objetos do projeto, exceto main.o)
TEST_OBJECTS = $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS))

$(BIN_DIR)/tests/%: tests/%.cpp $(TEST_OBJECTS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) $< $(TEST_OBJECTS) -I. -o $@

test: all $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "Running $$t"; \
		./$$t || { echo "Test failed: $$t"; exit 1; }; \
	done
	@echo "✓ Todos os testes passaram"

# Limpa arquivos compilados
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "✓ Limpeza concluída"

# Mostra ajuda
help:
	@echo "Targets disponíveis:"
	@echo "  make         - Compila o projeto"
	@echo "  make run     - Compila e executa"
	@echo "  make run ARGS=\"arquivo.class\" - Executa com argumentos para a main"
	@echo "  make clean   - Remove arquivos compilados"
	@echo "  make help    - Mostra esta mensagem"

# Debug: mostra variáveis
debug:
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"
	@echo "EXECUTABLE: $(EXECUTABLE)"

# Inclui dependências geradas automaticamente para headers
-include $(DEPS)
