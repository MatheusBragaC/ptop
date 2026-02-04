# ptop

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Language](https://img.shields.io/badge/language-C11-orange.svg)
![Build](https://img.shields.io/badge/build-CMake-green.svg)

**ptop** é um monitor de sistema de alta performance e minimalista para Linux, escrito em puro C11. Ele fornece uma Interface de Usuário em Texto (TUI) limpa para visualizar o uso da CPU, frequência, temperatura e carga do sistema em tempo real, projetado com princípios modernos de engenharia de software.

## Funcionalidades

*   **Monitoramento em Tempo Real**: Uso de CPU por núcleo, temperatura e frequência.
*   **Histórico Visual**: Gráficos baseados em caracteres Braille para dados históricos de alta resolução no terminal.
*   **Leve**: Pegada de memória mínima, zero dependências de execução externas.
*   **Arquitetura Moderna**:
    *   Núcleo orientado a eventos usando `epoll`, `timerfd` e `signalfd` (Zero polling).
    *   Clean Architecture: Separação estrita entre Modelo de Dados, Coletor (Abstração de Hardware) e UI.
    *   Componentes com Testes Unitários.

## Build e Instalação

### Dependências
*   **Compilador C** (GCC/Clang)
*   **CMake** (3.10+)

### Compilando a partir do Código Fonte

```bash
# Clonar o repositório
git clone https://github.com/pedroivo1/ptop.git
cd ptop

# Criar diretório de build
mkdir build && cd build

# Configurar e Compilar
cmake ..
make
```

### Executando

```bash
./ptop
```

## Arquitetura

O projeto segue uma arquitetura modular para garantir manutenibilidade e testabilidade:

*   **Core (`src/core`)**: Estruturas de dados puras e lógica de coleta de dados de hardware.
    *   `model.h`: Define o estado do `CpuModel`.
    *   `collector.h`: Interface para sensores de hardware.
    *   `parser.c`: **Funções puras** para parsing de arquivos do sistema (`/proc`, `/sys`), totalmente testadas unitariamente.
*   **UI (`src/ui`)**: Lógica de renderização usando buffering eficiente e sequências de escape ANSI.
*   **Main**: Orquestração do loop de eventos.

## Testes

Testes unitários são implementados usando CTest.

```bash
cd build
ctest --verbose
```

## Contribuindo

Contribuições são bem-vindas! Por favor, siga o estilo de código do projeto (definido em `.clang-format`) e garanta que todos os testes passem antes de enviar um PR.

1.  Faça um Fork do Projeto
2.  Crie sua Branch de Feature (`git checkout -b feature/FeatureIncrivel`)
3.  Commit suas Mudanças (`git commit -m 'Adiciona alguma FeatureIncrivel'`)
4.  Push para a Branch (`git push origin feature/FeatureIncrivel`)
5.  Abra um Pull Request

## 📄 Licença

Só um cara brincando um pouco