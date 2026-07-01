# JVM

JVM simplificada em C++ (trabalho acadêmico). O projeto tem duas frentes:

- **Leitor-Exibidor** — lê arquivos Java `.class` e exibe suas estruturas internas
  (constant pool, fields, methods, bytecodes desmontados, etc.). Binário: `bin/leitor-exibidor`.
- **Interpretador (Execution Engine)** — carrega um `.class`, localiza o método `main` e
  executa os bytecodes. Binário: `bin/jvm`.

> Estado atual: a infraestrutura de execução está montada (Class Loader, Runtime Data Area,
> loop de despacho), mas muitos opcodes do interpretador ainda são *stubs*. O Leitor-Exibidor
> é totalmente funcional.

Estrutura do projeto
--------------------

```
jvm/
├── main.cpp            # entry point do Leitor-Exibidor (exibe o .class)
├── jvm.cpp             # entry point do interpretador (executa o .class)
├── common/             # headers compartilhados (estrutura_dados, cp_utils, ...)
├── leitor/             # leitura binária big-endian do .class
├── parser/             # bytes do .class → class_info
├── exibidor/           # exibição + disassembler de bytecode
├── loader/             # ClassLoader + Method Area
├── runtime/            # Value, Frame, FrameStack, PC Register
├── heap/               # Heap (objetos e arrays)
└── interpreter/        # loop de execução e opcodes
```

Build e execução
-----------------

O mesmo `Makefile` funciona em **Linux/macOS** e em **Windows nativo** (Prompt de
Comando). Ele detecta o sistema operacional e usa os comandos certos de cada um
(`mkdir`/`rm` no Unix, `mkdir`/`rmdir` do `cmd.exe` no Windows) e gera `.exe`
automaticamente no Windows.

### Linux / macOS

Requisitos: `g++` (C++11) e `make`.

```bash
make
```

### Windows (Prompt de Comando, sem WSL)

Requisitos:

- **MinGW-w64** (fornece o `g++`). A forma mais simples é instalar o
  [MSYS2](https://www.msys2.org/) e depois, no terminal MSYS2:
  `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make`.
- Adicionar a pasta `bin` do MinGW ao **PATH** (ex.: `C:\msys64\ucrt64\bin`).

Abra o **Prompt de Comando (cmd.exe)** na raiz do projeto e use `mingw32-make`
(nome do `make` que vem com o MinGW):

```bat
mingw32-make
```

> Importante: rode no **cmd.exe**, não no Git Bash/MSYS. Assim as receitas usam
> os comandos nativos do Windows. Se o seu `make` chamar `mingw32-make`, pode
> trocar `mingw32-make` por `make` nos exemplos abaixo — o comportamento é o mesmo.

Os binários gerados são `bin\jvm.exe` e `bin\leitor-exibidor.exe`.

### Comandos (iguais nos dois sistemas)

**Executar um `.class` no interpretador:**

```bash
make exec exemplos/fatorial.class
```

Diretamente pelo binário — Linux/macOS:

```bash
./bin/jvm exemplos/fatorial.class
```

Diretamente pelo binário — Windows (cmd.exe):

```bat
bin\jvm.exe exemplos\fatorial.class
```

**Exibir o conteúdo de um `.class` (Leitor-Exibidor):**

```bash
make run ARGS="exemplos/Hello.class"
```

Diretamente — Linux/macOS: `./bin/leitor-exibidor exemplos/Hello.class`
· Windows: `bin\leitor-exibidor.exe exemplos\Hello.class`

Compilação manual (sem make)
-----------------------------

Como cada componente vive em sua subpasta de `jvm/`, é preciso passar os `-I` correspondentes.
Os comandos abaixo são para Linux/macOS; no Windows use `\` em vez de `/` na criação da
pasta (`if not exist bin mkdir bin`) e o `g++` do MinGW já produz `bin\jvm.exe`.

Leitor-Exibidor:

```bash
mkdir -p bin
g++ -std=c++11 -Wall -Wextra -O2 \
    -Ijvm/common -Ijvm/leitor -Ijvm/parser -Ijvm/exibidor \
    jvm/main.cpp jvm/leitor/leitor.cpp jvm/parser/parser.cpp \
    jvm/exibidor/exibidor.cpp jvm/exibidor/disasm.cpp \
    -o bin/leitor-exibidor
```

Interpretador:

```bash
mkdir -p bin
g++ -std=c++11 -Wall -Wextra -O2 \
    -Ijvm/common -Ijvm/leitor -Ijvm/parser -Ijvm/runtime \
    -Ijvm/loader -Ijvm/heap -Ijvm/interpreter \
    jvm/jvm.cpp jvm/interpreter/interpreter.cpp jvm/loader/class_loader.cpp \
    jvm/parser/parser.cpp jvm/leitor/leitor.cpp \
    -o bin/jvm
```

Debugar no VSCode
-----------------

Precisa da extensão **C/C++** da Microsoft (`ms-vscode.cpptools`) e do `gdb` instalado
(`sudo apt install gdb`). O repositório já traz `.vscode/tasks.json` e `.vscode/launch.json`
configurados.

1. Coloque *breakpoints* clicando na margem esquerda das linhas (ex.: em `op_new` no
   `jvm/interpreter/interpreter.cpp`).
2. Abra o painel **Run and Debug** (`Ctrl+Shift+D`) e escolha a configuração:
   - **Debug interpretador (bin/jvm)** — para depurar a execução de bytecode.
   - **Debug leitor-exibidor (bin/leitor-exibidor)** — para depurar a leitura/exibição.
3. Pressione **F5**. Antes de iniciar, ele roda a task `build-debug` (recompila com `-g -O0`)
   e pergunta o caminho do `.class` (padrão: `exemplos/fatorial.class`).

Atalhos durante a sessão: **F5** continua, **F10** passo sobre, **F11** entra na função,
**Shift+F11** sai da função. Variáveis e a pilha de chamadas aparecem no painel lateral.

> O importante é compilar com símbolos de debug. A task usa `make DEBUG=1`, que troca o `-O2`
> por `-g -O0` — sem isso o gdb não consegue fazer passo-a-passo de forma confiável.

Pela linha de comando, dá para depurar direto no `gdb`:

```bash
make DEBUG=1
gdb --args ./bin/jvm exemplos/fatorial.class
```

Testes
------

Os testes ficam em `tests/`. Para compilar e executar:

```bash
make test
```

Para rodar um teste específico depois de compilado:

```bash
./bin/tests/<nome_do_teste>
```

Limpeza
-------

```bash
make clean
```

Ajuda
-----

```bash
make help
```
