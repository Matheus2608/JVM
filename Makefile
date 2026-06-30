# Makefile para o projeto JVM
# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS =
DEPFLAGS = -MMD -MP

# Build de debug:  make DEBUG=1 ...
# Gera símbolos (-g) e desliga otimização (-O0) para passo-a-passo no gdb/VSCode.
ifeq ($(DEBUG),1)
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O0
endif

# Diretórios de include — todos os componentes em jvm/.
# Como os #include usam nomes simples (sem caminho), cada subpasta vira um -I.
INCLUDES = -Ijvm/common -Ijvm/leitor -Ijvm/parser -Ijvm/exibidor \
           -Ijvm/runtime -Ijvm/loader -Ijvm/heap -Ijvm/interpreter

# Diretórios
BIN_DIR = bin
OBJ_DIR = obj

# Arquivos fonte (caminhos relativos à raiz do projeto)
SOURCES = jvm/main.cpp \
          jvm/leitor/leitor.cpp \
          jvm/parser/parser.cpp \
          jvm/exibidor/exibidor.cpp \
          jvm/exibidor/disasm.cpp
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/leitor-exibidor

# Fontes do interpretador (Execution Engine) — gera o binário bin/jvm
JVM_SOURCES = jvm/jvm.cpp \
              jvm/interpreter/interpreter.cpp \
              jvm/loader/class_loader.cpp \
              jvm/parser/parser.cpp \
              jvm/leitor/leitor.cpp \
              jvm/heap/heap.cpp
JVM_OBJECTS = $(JVM_SOURCES:%.cpp=$(OBJ_DIR)/%.o)
JVM_EXEC = $(BIN_DIR)/jvm

DEPS = $(OBJECTS:.o=.d) $(JVM_OBJECTS:.o=.d)

# Test sources and bins
TEST_SOURCES = $(wildcard tests/*.cpp)
TEST_NAMES = $(notdir $(basename $(TEST_SOURCES)))
TEST_BINS = $(addprefix $(BIN_DIR)/tests/,$(TEST_NAMES))

# Targets
.PHONY: all clean run exec help test debug

all: $(EXECUTABLE) $(JVM_EXEC)

# Compilação do executável do Leitor-Exibidor (exibe o .class)
$(EXECUTABLE): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✓ Compilação concluída: $@"

# Compilação do interpretador (executa o .class)
$(JVM_EXEC): $(JVM_OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(JVM_OBJECTS) -o $@ $(LDFLAGS)
	@echo "✓ Compilação concluída: $@"

# Compilação dos arquivos objeto (espelha a árvore de jvm/ dentro de obj/)
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@
	@echo "✓ Compilado: $<"

# Cria diretórios se não existirem
$(BIN_DIR):
	@mkdir -p $@

$(BIN_DIR)/tests:
	@mkdir -p $@

# Exibe um .class (Leitor-Exibidor)
run: $(EXECUTABLE)
	@echo "Executando $(EXECUTABLE)..."
	@./$(EXECUTABLE) $(ARGS)

# Executa um .class no interpretador:  make exec exemplos/fatorial.class
# O nome do arquivo é capturado como "goal" extra da linha de comando.
EXEC_FILE := $(filter-out exec,$(MAKECMDGOALS))
exec: $(JVM_EXEC)
	@./$(JVM_EXEC) $(EXEC_FILE)

# Faz o .class passado virar um goal "no-op" (evita erro "No rule to make target").
ifneq ($(EXEC_FILE),)
$(EXEC_FILE):
	@:
endif

# Compila e executa testes em tests/ (linka os objetos do projeto, exceto main.o)
TEST_OBJECTS = $(filter-out $(OBJ_DIR)/jvm/main.o, $(OBJECTS))

$(BIN_DIR)/tests/%: tests/%.cpp $(TEST_OBJECTS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(TEST_OBJECTS) -o $@

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
	@echo "  make                         - Compila tudo (leitor-exibidor + jvm)"
	@echo "  make exec exemplos/X.class   - Executa um .class no interpretador"
	@echo "  make run ARGS=\"X.class\"      - Exibe o conteúdo de um .class (Leitor-Exibidor)"
	@echo "  make test                    - Compila e roda os testes"
	@echo "  make clean                   - Remove arquivos compilados"
	@echo "  make help                    - Mostra esta mensagem"

# Debug: mostra variáveis
debug:
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"
	@echo "EXECUTABLE: $(EXECUTABLE)"

# Inclui dependências geradas automaticamente para headers
-include $(DEPS)
