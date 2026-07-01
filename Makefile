# Makefile para o projeto JVM
#
# Multiplataforma: funciona igual em Linux/macOS (shell sh) e em Windows nativo
# (Prompt de Comando cmd.exe + MinGW). A detecção usa a variável de ambiente OS,
# que o Windows sempre define como "Windows_NT".
#
# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -D_GLIBCXX_USE_CXX11_ABI=0
LDFLAGS = -static-libstdc++
DEPFLAGS = -MMD -MP

# Build de debug:  make DEBUG=1 ...
# Gera símbolos (-g) e desliga otimização (-O0) para passo-a-passo no gdb/VSCode.
ifeq ($(DEBUG),1)
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O0
endif

# -----------------------------------------------------------------------------
# Detecção de plataforma
#   EXE     — sufixo do executável (.exe no Windows, vazio no Unix)
#   RUN     — prefixo para executar um binário local (vazio no Windows, ./ no Unix)
#   MKDIR   — cria um diretório (com os pais) — $(call MKDIR,caminho)
#   RMDIR   — remove um diretório recursivamente — $(call RMDIR,caminho)
#   fixpath — troca / por \ nos caminhos passados a comandos do cmd.exe
# -----------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    # Força o cmd.exe como shell mesmo que exista sh no PATH (MSYS/Git).
    SHELL   := cmd.exe
    .SHELLFLAGS := /c
    EXE     := .exe
    RUN     :=
    NOOP    := cd .
    fixpath  = $(subst /,\,$1)
    MKDIR    = if not exist "$(call fixpath,$1)" mkdir "$(call fixpath,$1)"
    RMDIR    = if exist "$(call fixpath,$1)" rmdir /s /q "$(call fixpath,$1)"
else
    EXE     :=
    RUN     := ./
    NOOP    := :
    fixpath  = $1
    MKDIR    = mkdir -p $1
    RMDIR    = rm -rf $1
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
EXECUTABLE = $(BIN_DIR)/leitor-exibidor$(EXE)

# Fontes do interpretador (Execution Engine) — gera o binário bin/jvm
JVM_SOURCES = jvm/jvm.cpp \
              jvm/interpreter/interpreter.cpp \
              jvm/loader/class_loader.cpp \
              jvm/parser/parser.cpp \
              jvm/leitor/leitor.cpp
JVM_OBJECTS = $(JVM_SOURCES:%.cpp=$(OBJ_DIR)/%.o)
JVM_EXEC = $(BIN_DIR)/jvm$(EXE)

DEPS = $(OBJECTS:.o=.d) $(JVM_OBJECTS:.o=.d)

# Test sources and bins
TEST_SOURCES = $(wildcard tests/*.cpp)
TEST_NAMES = $(notdir $(basename $(TEST_SOURCES)))
TEST_BINS = $(addprefix $(BIN_DIR)/tests/,$(addsuffix $(EXE),$(TEST_NAMES)))

# Targets
.PHONY: all clean run exec help test debug

all: $(EXECUTABLE) $(JVM_EXEC)

# Compilação do executável do Leitor-Exibidor (exibe o .class)
$(EXECUTABLE): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo Compilacao concluida: $@

# Compilação do interpretador (executa o .class)
$(JVM_EXEC): $(JVM_OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(JVM_OBJECTS) -o $@ $(LDFLAGS)
	@echo Compilacao concluida: $@

# Compilação dos arquivos objeto (espelha a árvore de jvm/ dentro de obj/)
$(OBJ_DIR)/%.o: %.cpp
	@$(call MKDIR,$(patsubst %/,%,$(dir $@)))
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@
	@echo Compilado: $<

# Cria diretórios se não existirem
$(BIN_DIR):
	@$(call MKDIR,$(BIN_DIR))

$(BIN_DIR)/tests:
	@$(call MKDIR,$(BIN_DIR)/tests)

# Exibe um .class (Leitor-Exibidor)
run: $(EXECUTABLE)
	@echo Executando $(EXECUTABLE)...
	@$(RUN)$(call fixpath,$(EXECUTABLE)) $(ARGS)

# Executa um .class no interpretador:  make exec exemplos/fatorial.class
# O nome do arquivo é capturado como "goal" extra da linha de comando.
# Filtra todos os alvos .PHONY para não transformar "make help" (etc.) num no-op.
EXEC_FILE := $(filter-out all clean run exec help test debug,$(MAKECMDGOALS))
exec: $(JVM_EXEC)
	@$(RUN)$(call fixpath,$(JVM_EXEC)) $(EXEC_FILE)

# Faz o .class passado virar um goal "no-op" (evita erro "No rule to make target").
ifneq ($(EXEC_FILE),)
$(EXEC_FILE):
	@$(NOOP)
endif

# Compila e executa testes em tests/ (linka os objetos do projeto, exceto main.o)
TEST_OBJECTS = $(filter-out $(OBJ_DIR)/jvm/main.o, $(OBJECTS))

$(BIN_DIR)/tests/%$(EXE): tests/%.cpp $(TEST_OBJECTS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(TEST_OBJECTS) -o $@

# Roda todos os testes encadeando com && (para no primeiro que falhar).
# O $(foreach) monta uma única linha de comando, o que funciona tanto em sh
# quanto no cmd.exe — sem depender da sintaxe de laço de nenhum dos dois.
test: all $(TEST_BINS)
	@$(foreach t,$(TEST_BINS),echo Running $(t) && $(RUN)$(call fixpath,$(t)) &&) echo Todos os testes passaram

# Limpa arquivos compilados
clean:
	@$(call RMDIR,$(OBJ_DIR))
	@$(call RMDIR,$(BIN_DIR))
	@echo Limpeza concluida.

# Mostra ajuda
help:
	@echo Targets disponiveis:
	@echo   make                         - Compila tudo: leitor-exibidor e jvm
	@echo   make exec exemplos/X.class   - Executa um .class no interpretador
	@echo   make run ARGS=X.class        - Exibe o conteudo de um .class com o Leitor-Exibidor
	@echo   make test                    - Compila e roda os testes
	@echo   make clean                   - Remove arquivos compilados
	@echo   make help                    - Mostra esta mensagem

# Debug: mostra variáveis
debug:
	@echo SOURCES: $(SOURCES)
	@echo OBJECTS: $(OBJECTS)
	@echo EXECUTABLE: $(EXECUTABLE)

# Inclui dependências geradas automaticamente para headers
-include $(DEPS)
